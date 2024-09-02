// Copyright 2019 Ant Group Co., Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "channel_brpc_gateway.h"
#include "interconnection/link/transport.pb.h"
#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"

#include "spdlog/spdlog.h"

namespace yacl::link {

namespace ic = org::interconnection;
namespace ic_pb = org::interconnection::link;
namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;

namespace {

// TODO: move this to somewhere-else.
class BatchDesc {
 protected:
  size_t batch_idx_;
  size_t batch_size_;
  size_t total_size_;

 public:
  BatchDesc(size_t batch_idx, size_t batch_size, size_t total_size)
      : batch_idx_(batch_idx),
        batch_size_(batch_size),
        total_size_(total_size) {}

  // return the index of this batch.
  size_t Index() const { return batch_idx_; }

  // return the offset of the first element in this batch.
  size_t Begin() const { return batch_idx_ * batch_size_; }

  // return the offset after last element in this batch.
  size_t End() const { return std::min(Begin() + batch_size_, total_size_); }

  // return the size of this batch.
  size_t Size() const { return End() - Begin(); }

  std::string ToString() const { return "B:" + std::to_string(batch_idx_); };
};

class OnGatewaySendDone : public google::protobuf::Closure {
 public:
  explicit OnGatewaySendDone(std::shared_ptr<ChannelBrpcGateWay> channel)
      : channel_(std::move(channel)) {
    channel_->AddAsyncCount();
  }

  ~OnGatewaySendDone() override {
    try {
      channel_->SubAsyncCount();
    } catch (std::exception& e) {
      SPDLOG_INFO(e.what());
    }
  }

  void Run() override {
    std::unique_ptr<OnGatewaySendDone> self_guard(this);

    if (cntl_.Failed()) {
      SPDLOG_WARN("send, rpc failed={}, message={}", cntl_.ErrorCode(),
                  cntl_.ErrorText());
    } else {
      // SPDLOG_WARN("send, peer failed code={} message={} sessionId={} has_content={}"
      // ,response_.code(),response_.message(),response_.sessionid(),response_.has_content());
      if(response_.has_content()) 
      {
        auto& bytedata = response_.content().objectbytedata();
        ic_pb::PushResponse push_response;
        push_response.ParseFromArray(bytedata.c_str(),bytedata.size());
        if (push_response.header().error_code() != ic::ErrorCode::OK) {
          SPDLOG_WARN("send, peer failed message={}",push_response.header().error_msg());
        }
      }
    }
  }

  gateway_pb::ReturnStatus response_;
  brpc::Controller cntl_;
  std::shared_ptr<ChannelBrpcGateWay> channel_;
};

}  // namespace

void ChannelBrpcGateWay::SetPeerHost(const std::string& peer_host) {
  auto brpc_channel = std::make_unique<brpc::Channel>();
  const auto load_balancer = "";
  brpc::ChannelOptions options;
  {
    options.protocol = options_.channel_protocol;
    options.connection_type = options_.channel_connection_type;
    options.connect_timeout_ms = 20000;
    options.timeout_ms = options_.http_timeout_ms;
    options.max_retry = 3;
    // options.retry_policy = DefaultRpcRetryPolicy();
  }
  int res = brpc_channel->Init(peer_host.c_str(), load_balancer, &options);
  if (res != 0) {
    YACL_THROW_NETWORK_ERROR("Fail to initialize channel, host={}, err_code={}",
                             peer_host, res);
  }

  channel_ = std::move(brpc_channel);
  peer_host_ = peer_host;
}

namespace {

struct SendChunckedBrpcTask {
  std::shared_ptr<ChannelBrpcGateWay> channel;
  std::string key;
  Buffer value;

  SendChunckedBrpcTask(std::shared_ptr<ChannelBrpcGateWay> _channel, std::string _key,
                       Buffer _value)
      : channel(std::move(_channel)),
        key(std::move(_key)),
        value(std::move(_value)) {
    channel->AddAsyncCount();
  }

  ~SendChunckedBrpcTask() {
    try {
      channel->SubAsyncCount();
    } catch (std::exception& e) {
      SPDLOG_INFO(e.what());
    }
  }

  static void* Proc(void* args) {
    // take ownership of task.
    std::unique_ptr<SendChunckedBrpcTask> task(
        static_cast<SendChunckedBrpcTask*>(args));

    task->channel->SendChunked(task->key, task->value);
    return nullptr;
  }
};

}  // namespace

void ChannelBrpcGateWay::AddAsyncCount() {
  std::unique_lock<bthread::Mutex> lock(wait_async_mutex_);
  running_async_count_++;
}

void ChannelBrpcGateWay::SubAsyncCount() {
  std::unique_lock<bthread::Mutex> lock(wait_async_mutex_);
  YACL_ENFORCE(running_async_count_ > 0);
  running_async_count_--;
  if (running_async_count_ == 0) {
    wait_async_cv_.notify_all();
  }
}

void ChannelBrpcGateWay::WaitAsyncSendToFinish() {
  std::unique_lock<bthread::Mutex> lock(wait_async_mutex_);
  while (running_async_count_ > 0) {
    wait_async_cv_.wait(lock);
  }
}

template <class ValueType>
void ChannelBrpcGateWay::SendAsyncInternal(const std::string& key, ValueType&& value) {
  if (value.size() > options_.http_max_payload_size) {
    auto btask = std::make_unique<SendChunckedBrpcTask>(
        this->shared_from_this(), key, Buffer(std::forward<ValueType>(value)));

    // bthread run in 'detached' mode, we will never wait for it.
    bthread_t tid;
    if (bthread_start_background(&tid, nullptr, SendChunckedBrpcTask::Proc,
                                 btask.get()) == 0) {
      // bthread takes the ownership, release it.
      static_cast<void>(btask.release());
    } else {
      YACL_THROW("failed to push async sending job to bthread");
    }

    return;
  }

  ic_pb::PushRequest request;
  {
    request.set_sender_rank(self_rank_);
    request.set_key(key);
    request.set_value(reinterpret_cast<const char*>(value.data()),
                      value.size());
    request.set_trans_type(ic_pb::TransType::MONO);
  }

  gateway_pb::TransferMeta gateway_request;
  gateway_request.set_sessionid(SessionId());
  gateway_request.set_processor(GATEWAY_PROCESSOR);
  gateway_request.mutable_src()->set_memberid(SelfMemberId());
  gateway_request.mutable_dst()->set_memberid(PeerMemberId());
  // SPDLOG_INFO("before set gateway_request size:{}, ic_pb size:{}, value size:{} maxsize:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size(), options_.http_max_payload_size);
  gateway_request.mutable_content()->set_objectdata(SPU_SERVICE_TYPE);
  gateway_request.mutable_content()->set_objectbytedata(request.SerializeAsString());
  // SPDLOG_INFO("after set gateway_request size:{}, ic_pb size:{}, value size:{} maxsize:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size(), options_.http_max_payload_size);

  OnGatewaySendDone* done = new OnGatewaySendDone(shared_from_this());
  // ic_pb::ReceiverService::Stub stub(channel_.get());
  gateway_service::TransferService::Stub stub(channel_.get());
  stub.send(&done->cntl_, &gateway_request, &done->response_, done);
}

void ChannelBrpcGateWay::SendAsyncImpl(const std::string& key, Buffer&& value) {
  SendAsyncInternal(key, std::move(value));
}

void ChannelBrpcGateWay::SendAsyncImpl(const std::string& key,
                                ByteContainerView value) {
  SendAsyncInternal(key, value);
}

void ChannelBrpcGateWay::SendImpl(const std::string& key, ByteContainerView value) {
  SendImpl(key, value, 0);
}

void ChannelBrpcGateWay::SendImpl(const std::string& key, ByteContainerView value,
                           uint32_t timeout) {
  // SPDLOG_INFO("ChannelBrpcGateWay::SendImpl key:{} value:{}",key,value);
  if (value.size() > options_.http_max_payload_size) {
    SendChunked(key, value);
    return;
  }
  ic_pb::PushRequest request;
  {
    request.set_sender_rank(self_rank_);
    request.set_key(key);
    request.set_value(value.data(), value.size());
    request.set_trans_type(ic_pb::TransType::MONO);
  }

  gateway_pb::TransferMeta gateway_request;
  gateway_request.set_processor(GATEWAY_PROCESSOR); 
  gateway_request.set_sessionid(SessionId());
  gateway_request.mutable_src()->set_memberid(SelfMemberId());
  gateway_request.mutable_dst()->set_memberid(PeerMemberId());
  // SPDLOG_INFO("before set gateway_request size:{}, ic_pb size:{}, value size:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size());
  gateway_request.mutable_content()->set_objectdata(SPU_SERVICE_TYPE);
  gateway_request.mutable_content()->set_objectbytedata(request.SerializeAsString());
  // SPDLOG_INFO("after set gateway_request size:{}, ic_pb size:{}, value size:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size());

  brpc::Controller cntl;
  if (timeout != 0) {
    cntl.set_timeout_ms(timeout);
  }
  gateway_pb::ReturnStatus gateway_response;
  gateway_service::TransferService::Stub stub(channel_.get());
  stub.send(&cntl, &gateway_request, &gateway_response, nullptr);

  // handle failures.
  if (cntl.Failed()) {
    YACL_THROW_NETWORK_ERROR("ChannelBrpcGateWay send, rpc failed={}, message={}",
                             cntl.ErrorCode(), cntl.ErrorText());
  }
  if(gateway_response.code() != 0) {
      SPDLOG_INFO("ChannelBrpcGateWay send, gate failed  code={}, sessionid={},message={}",
                             gateway_response.code(),gateway_response.sessionid() ,gateway_response.message());
      YACL_THROW_NETWORK_ERROR("ChannelBrpcGateWay send, gate failed  code={}, sessionid={},message={}",
                             gateway_response.code(),gateway_response.sessionid() ,gateway_response.message());
  }

  // if (response.header().error_code() != ic::ErrorCode::OK) {
  //   YACL_THROW("send, peer failed message={}", response.header().error_msg());
  // }
  if(gateway_response.has_content()) 
  {
    auto& bytedata = gateway_response.content().objectbytedata();
    ic_pb::PushResponse push_response;
    push_response.ParseFromArray(bytedata.c_str(),bytedata.size());
    if (push_response.header().error_code() != ic::ErrorCode::OK) {
      YACL_THROW("send, peer failed message={}", push_response.header().error_msg());
    }
  }
}

// See: chunked streamming
//   https://en.wikipedia.org/wiki/Chunked_transfer_encoding
// See: Brpc does NOT support POST chunked.
//   https://github.com/apache/incubator-brpc/blob/master/docs/en/http_client.md
void ChannelBrpcGateWay::SendChunked(const std::string& key, ByteContainerView value) {
  const size_t bytes_per_chunk = options_.http_max_payload_size;
  const size_t num_bytes = value.size();
  const size_t num_chunks = (num_bytes + bytes_per_chunk - 1) / bytes_per_chunk;

  constexpr uint32_t kParallelSize = 10;
  const size_t batch_size = kParallelSize;
  const size_t num_batches = (num_chunks + batch_size - 1) / batch_size;

  for (size_t batch_idx = 0; batch_idx < num_batches; batch_idx++) {
    const BatchDesc batch(batch_idx, batch_size, num_chunks);

    // See: "半同步“ from
    // https://github.com/apache/incubator-brpc/blob/master/docs/cn/client.md
    std::vector<brpc::Controller> cntls(batch.Size());
    std::vector<gateway_pb::ReturnStatus> responses(batch.Size());

    // fire batched chunk requests.
    for (size_t idx = 0; idx < batch.Size(); idx++) {
      const size_t chunk_idx = batch.Begin() + idx;
      const size_t chunk_offset = chunk_idx * bytes_per_chunk;

      ic_pb::PushRequest request;
      {
        request.set_sender_rank(self_rank_);
        request.set_key(key);
        request.set_value(
            value.data() + chunk_offset,
            std::min(bytes_per_chunk, value.size() - chunk_offset));
        request.set_trans_type(ic_pb::TransType::CHUNKED);
        request.mutable_chunk_info()->set_chunk_offset(chunk_offset);
        request.mutable_chunk_info()->set_message_length(num_bytes);
      }

      gateway_pb::TransferMeta gateway_request;
      gateway_request.set_sessionid(SessionId());
      gateway_request.set_processor(GATEWAY_PROCESSOR);
      gateway_request.mutable_src()->set_memberid(SelfMemberId());
      gateway_request.mutable_dst()->set_memberid(PeerMemberId());
      // SPDLOG_INFO("before set gateway_request size:{}, ic_pb size:{}, value size:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size());
      gateway_request.mutable_content()->set_objectdata(SPU_SERVICE_TYPE);
      gateway_request.mutable_content()->set_objectbytedata(request.SerializeAsString());
      // SPDLOG_INFO("after set gateway_request size:{}, ic_pb size:{}, value size:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size());
      auto& cntl = cntls[idx];
      auto& response = responses[idx];
      gateway_service::TransferService::Stub stub(channel_.get());
      stub.send(&cntl, &gateway_request, &response, brpc::DoNothing());
    }

    for (size_t idx = 0; idx < batch.Size(); idx++) {
      brpc::Join(cntls[idx].call_id());
    }

    for (size_t idx = 0; idx < batch.Size(); idx++) {
      const size_t chunk_idx = batch.Begin() + idx;
      const auto& cntl = cntls[idx];
      const auto& response = responses[idx];
      if (cntl.Failed()) {
        YACL_THROW_NETWORK_ERROR(
            "ChannelBrpcGateWay send key={} (chunked {} out of {}) rpc failed: {}, message={}",
            key, chunk_idx + 1, num_chunks, cntl.ErrorCode(), cntl.ErrorText());
      } else if(response.code() != 0) {
        YACL_THROW_NETWORK_ERROR("ChannelBrpcGateWay send, gate failed  code={}, sessionid={},message={}",
                             response.code(),response.sessionid() ,response.message());
      } else {
        if(response.has_content()) 
        {
          auto& bytedata = response.content().objectbytedata();
          ic_pb::PushResponse push_response;
          push_response.ParseFromArray(bytedata.c_str(),bytedata.size());
          if (push_response.header().error_code() != ic::ErrorCode::OK) {
            YACL_THROW(
              "ChannelBrpcGateWay send key={} (chunked {} out of {}) response failed, message={}",
             key, chunk_idx + 1, num_chunks, push_response.header().error_msg());
          }
        }
      }
    }
  }
}

}  // namespace yacl::link

