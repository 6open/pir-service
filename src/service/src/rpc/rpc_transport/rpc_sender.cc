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

#include "rpc_sender.h"
#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"

#include "spdlog/spdlog.h"
#include "yacl/base/exception.h"    


namespace mpc {
namespace rpc {

namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;

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

void RpcSender::SetPeerHost(const std::string& peer_host)
{
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
  // brpc_channel->options().connection_type = "persistent";
  channel_ = std::move(brpc_channel);
  peer_host_ = peer_host;
}
void RpcSender::Send(const std::string& msg_key, const std::string& message,uint32_t timeout)
{
    std::unique_lock<bthread::Mutex> lock(send_mutex_);
    std::string convert_key = GeneratorKey(msg_key);
    SendImpl(convert_key,message,timeout);
}

std::string RpcSender::GeneratorKey(const std::string& msg_key)
{
    return rpc_key_adpapter_.Mask(rpc_seq_sender_.BuildKey(msg_key));
}

void RpcGatewaySender::SendImpl(const std::string& key, const std::string& value,uint32_t timeout) 
{
    if(value.size() >= options_.http_max_payload_size){
      SendChunked(key,value);
      return;
    }
    gateway_pb::TransferMeta gateway_request;
    gateway_request.set_processor(GATEWAY_PROCESSOR); 
    gateway_request.set_sessionid(gateway_parma_.session_id);
    gateway_request.mutable_src()->set_memberid(gateway_parma_.self_memberid);
    gateway_request.mutable_dst()->set_memberid(gateway_parma_.peer_memberid);
    // SPDLOG_INFO("before set gateway_request size:{}, ic_pb size:{}, value size:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size());
    gateway_request.mutable_content()->set_objectdata(key);
    gateway_request.mutable_content()->set_objectbytedata(value);
    gateway_pb::ConfigData* config_data_type = gateway_request.mutable_content()->add_configdatas();
    config_data_type->set_key("msgtype");
    config_data_type->set_value("normal");
  // handle failures.
    brpc::Controller cntl;
    if (timeout != 0) {
        cntl.set_timeout_ms(timeout);
    }
    gateway_pb::ReturnStatus gateway_response;
    gateway_service::TransferService::Stub stub(channel_.get());
    stub.send(&cntl, &gateway_request, &gateway_response, nullptr);

    // handle failures.
    if (cntl.Failed()) {
    YACL_THROW_NETWORK_ERROR("RpcGatewaySender send, rpc failed={}, message={}",
                                cntl.ErrorCode(), cntl.ErrorText());
    }
    if(gateway_response.code() != 0) {
        SPDLOG_INFO("RpcGatewaySender send, gate failed  code={}, sessionid={},message={}",
                                gateway_response.code(),gateway_response.sessionid() ,gateway_response.message());
        YACL_THROW_NETWORK_ERROR("RpcGatewaySender send, gate failed  code={}, sessionid={},message={}",
                                gateway_response.code(),gateway_response.sessionid() ,gateway_response.message());
    }
}

void RpcGatewaySender::SendChunked(const std::string& key, const std::string& value) 
{
  const size_t bytes_per_chunk = options_.http_max_payload_size;
  const size_t num_bytes = value.size();
  const size_t num_chunks = (num_bytes + bytes_per_chunk - 1) / bytes_per_chunk;

  constexpr uint32_t kParallelSize = 10;
  const size_t batch_size = kParallelSize;
  const size_t num_batches = (num_chunks + batch_size - 1) / batch_size;
  SPDLOG_INFO("SendChunked key:{}, bytes_per_chunk:{} num_bytes:{}  num_chunks={}, batch_size={},num_batches={}",
                                key,bytes_per_chunk,num_bytes,
                                num_chunks,batch_size ,num_batches);
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

      gateway_pb::TransferMeta gateway_request;
      gateway_request.set_processor(GATEWAY_PROCESSOR); 
      gateway_request.set_sessionid(gateway_parma_.session_id);
      gateway_request.mutable_src()->set_memberid(gateway_parma_.self_memberid);
      gateway_request.mutable_dst()->set_memberid(gateway_parma_.peer_memberid);
      // SPDLOG_INFO("before set gateway_request size:{}, ic_pb size:{}, value size:{}",gateway_request.SerializeAsString().size(),request.SerializeAsString().size(),value.size());
      gateway_request.mutable_content()->set_objectdata(key);
      gateway_request.mutable_content()->set_objectbytedata(value);
      gateway_request.mutable_content()->set_objectbytedata(value.data() + chunk_offset,std::min(bytes_per_chunk, value.size() - chunk_offset));
      gateway_pb::ConfigData* config_data_type = gateway_request.mutable_content()->add_configdatas();
      config_data_type->set_key("msgtype");
      config_data_type->set_value("chunked");
      gateway_pb::ConfigData* config_data_len = gateway_request.mutable_content()->add_configdatas();
      config_data_len->set_key("len");
      config_data_len->set_value(std::to_string(num_bytes));
      gateway_pb::ConfigData* config_data_offset = gateway_request.mutable_content()->add_configdatas();
      config_data_offset->set_key("offset");
      config_data_offset->set_value(std::to_string(chunk_offset));
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
            "RpcGatewaySender::SendChunked send key={} (chunked {} out of {}) rpc failed: {}, message={}",
            key, chunk_idx + 1, num_chunks, cntl.ErrorCode(), cntl.ErrorText());
      } else if(response.code() != 0) {
        YACL_THROW_NETWORK_ERROR("RpcGatewaySender::SendChunked, gate failed  code={}, sessionid={},message={}",
                             response.code(),response.sessionid() ,response.message());
      } else {
          //succeed to nothing
      }
    }
  }
}

} // namespace rpc
} // namespace mpc