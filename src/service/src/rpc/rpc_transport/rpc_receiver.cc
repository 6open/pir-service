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

#include "rpc_receiver.h"
#include "spdlog/spdlog.h"
#include "yacl/base/exception.h"
#include "yacl/base/buffer.h"

namespace mpc {
namespace rpc {

class ChunkedMessage {
 public:
  explicit ChunkedMessage(int64_t message_length) : message_(message_length) {}

  void AddChunk(int64_t offset, const std::string& data) {
    std::unique_lock<bthread::Mutex> lock(mutex_);
    if (received_.emplace(offset).second) {
      std::memcpy(message_.data<std::byte>() + offset, data.data(),
                  data.size());
      bytes_written_ += data.size();
    }
  }

  bool IsFullyFilled() {
    std::unique_lock<bthread::Mutex> lock(mutex_);
    return bytes_written_ == message_.size();
  }

  yacl::Buffer&& Reassemble() {
    std::unique_lock<bthread::Mutex> lock(mutex_);
    return std::move(message_);
  }

 protected:
  bthread::Mutex mutex_;
  std::set<int64_t> received_;
  // chunk index to value.
  int64_t bytes_written_{0};
  yacl::Buffer message_;
};

RpcReceiver::RpcReceiver(const std::string& key)
:key_(key)
{
  SPDLOG_INFO("RpcReceiver::RpcReceiver key:{}, this {}",key_,fmt::ptr(static_cast<void*>(this)));
}

RpcReceiver::~RpcReceiver()
{
  SPDLOG_INFO("RpcReceiver::~RpcReceiver key:{}, this {}",key_,fmt::ptr(static_cast<void*>(this)));
}

void RpcReceiver::OnNormalMessage(const std::string& key, const std::string_view& message) 
{
  std::string msg_key;
  size_t seq_id = 0;

  std::tie(msg_key, seq_id) = seq_receiver_.SplitKey(key);
  // SPDLOG_INFO("OnNormalMessage msg_key:{}, seq_id:{},message:{}",msg_key,seq_id,message);
  if (seq_id > 0 && !seq_receiver_.Insert(seq_id)) {
    // 0 seq id use for TestSend/TestRecv, skip duplicate test.
    // Duplicate seq id found. may cause by rpc retry, ignore
    SPDLOG_WARN("Duplicate seq_id found, key {} seq_id {}", msg_key, seq_id);
    return;
  }

  if (!waiting_finish_.load()) {
    // SPDLOG_INFO("OnNormalMessage  loading msg_key:{}, seq_id:{},message:{} this {}",msg_key,seq_id,message,fmt::ptr(static_cast<void*>(this)));
    auto pair =
        msg_db_.emplace(msg_key, std::make_pair(message, seq_id));
    if (seq_id > 0 && !pair.second) {
      YACL_THROW(
          "For developer: BUG! PLS do not use same key for multiple msg, "
          "Duplicate key {} with new seq_id {}, old seq_id {}.",
          msg_key, seq_id, pair.first->second.second);
    }
  } else {
    // SendAck(seq_id);
    SPDLOG_WARN("Asymmetric logic exist, auto ack key {} seq_id {}", msg_key,
                seq_id);
  }
  msg_db_cond_.notify_all();
}

void RpcReceiver::OnMessage(const std::string& key, const std::string& message,std::map<std::string, std::string>& msg_config) {
    SPDLOG_INFO("OnMessage key:{}",key);
    std::unique_lock<bthread::Mutex> lock(msg_mutex_);
    OnNormalMessage(key, message);
}

std::string RpcReceiver::Recv(const std::string& msg_key) {

//   NormalMessageKeyEnforce(msg_key);
  // SPDLOG_INFO("msg_key:{},recv_timeout_ms_:{}",msg_key,recv_timeout_ms_);
  std::string value;
  size_t seq_id = 0;
  {
    // SPDLOG_INFO("Recv0 msg_key:{} this:{}",msg_key,fmt::ptr(static_cast<void*>(this)));
    std::unique_lock<bthread::Mutex> lock(msg_mutex_);
    // SPDLOG_INFO("1 msg_key:{}",msg_key);

    auto stop_waiting = [&] {
      // SPDLOG_INFO("in stop_waiting msg_key:{}",msg_key);
      auto itr = this->msg_db_.find(msg_key);
      if (itr == this->msg_db_.end()) {
        return false;
      } else {
        std::tie(value, seq_id) = std::move(itr->second);
        this->msg_db_.erase(itr);
        return true;
      }
    };
    // SPDLOG_INFO("2 msg_key:{}",msg_key);
    while (!stop_waiting()) {
      //                              timeout_us
      // SPDLOG_INFO("stop_waiting msg_key:{}",msg_key);

      if (msg_db_cond_.wait_for(lock, recv_timeout_ms_ * 1000) == ETIMEDOUT) {
        YACL_THROW_IO_ERROR("Get data timeout, key={}", msg_key);
      }
    }
  }
//   SendAck(seq_id);
  return value;
}

void RpcReceiver::OnChunkedMessage(const std::string& key,
                                   const std::string& value, size_t offset,
                                   size_t total_length) {
  // SPDLOG_INFO("OnChunkedMessage key:{},value sz:{},offset:{},total_length:{}",key,value.size(),offset,total_length);

  if (offset + value.size() > total_length) {
    YACL_THROW_LOGIC_ERROR(
        "invalid chunk info, offset={}, chun size = {}, total_length={}",
        offset, value.size(), total_length);
  }

  bool should_reassemble = false;
  std::shared_ptr<ChunkedMessage> data;
  {
    std::unique_lock<bthread::Mutex> lock(chunked_values_mutex_);
    auto itr = chunked_values_.find(key);
    if (itr == chunked_values_.end()) {
      itr = chunked_values_
                .emplace(key, std::make_shared<ChunkedMessage>(total_length))
                .first;
    }

    data = itr->second;
    data->AddChunk(offset, value);

    if (data->IsFullyFilled()) {
      chunked_values_.erase(itr);

      // only one thread do the reassemble
      should_reassemble = true;
    }
  }

  if (should_reassemble) {
    // notify new value arrived.
    std::unique_lock<bthread::Mutex> lock(msg_mutex_);
    yacl::Buffer buffer = data->Reassemble();
    OnNormalMessage(key, buffer);
  }
}

} // namespace rpc
} // namespace mpc