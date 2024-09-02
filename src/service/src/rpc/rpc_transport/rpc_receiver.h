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

#pragma once
#include <string>
#include <map>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>

#include "bthread/bthread.h"
#include "bthread/condition_variable.h"
#include "rpc_key_adpapter.h"
namespace mpc {
namespace rpc {
class ChunkedMessage;
class RpcReceiver
{
public:
    RpcReceiver(const std::string& key);
    ~RpcReceiver();
    void OnMessage(const std::string& key, const std::string& message,std::map<std::string, std::string>& msg_config);
    void OnChunkedMessage(const std::string& key,const std::string& value, size_t offset,size_t total_length);
    std::string Recv(const std::string& msg_key);

    void SetRecvTimeout(uint32_t recv_timeout_ms) {recv_timeout_ms_ = recv_timeout_ms;} 
    uint32_t GetRecvTimeout() const {return recv_timeout_ms_;};
private:
    void OnNormalMessage(const std::string& key, const std::string_view& message);
private:
    // std::map<std::string,std::string> msg_db_;
    std::string key_;
    std::map<std::string, std::pair<std::string, size_t>> msg_db_;
    bthread::Mutex msg_mutex_;
    bthread::ConditionVariable msg_db_cond_;

    std::atomic<bool> waiting_finish_ = false;
    std::set<std::size_t> received_msg_ids_;
    RpcSeqReceiver seq_receiver_;

    uint32_t recv_timeout_ms_ = 3 * 60 * 1000;  // 3 minites
    bthread::Mutex chunked_values_mutex_;
    std::map<std::string, std::shared_ptr<ChunkedMessage>> chunked_values_;

};

} // namespace rpc
} // namespace mpc