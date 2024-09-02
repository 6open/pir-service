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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

#include "rpc_key_adpapter.h"

#include "brpc/channel.h"
#include "brpc/server.h"
#include "bthread/bthread.h"
#include "bthread/condition_variable.h"

namespace mpc {
namespace rpc {
struct GatewayPrama
{
    std::string self_memberid;
    std::string peer_memberid;
    std::string session_id;
    std::string task_id;
};

class RpcSender 
{
 public:
  struct Options {
    uint32_t http_timeout_ms = 10 * 1000;         // 10 seconds
    uint32_t http_max_payload_size = 64 * 1024;  // 512k bytes
    std::string channel_protocol = "h2:grpc";
    std::string channel_connection_type = "single";
  };

 private:
  virtual void SendImpl(const std::string& key, const std::string& message,uint32_t timeout) = 0 ;

 public:
  // RpcSender(size_t self_rank, size_t peer_rank,const std::string& key, Options options)
  // :options_(options),rpc_key_adpapter_(key)
  //   {}

  RpcSender(Options options,const RpcKeyAdpapter& adpapter)
  :options_(options),rpc_key_adpapter_(adpapter)
    {}

  virtual ~RpcSender() = default;

  void SetPeerHost(const std::string& peer_host);
  void Send(const std::string& key, const std::string& message,uint32_t timeout=0);

  // max payload size for a single http request, in bytes.
  uint32_t GetHttpMaxPayloadSize() const {
    return options_.http_max_payload_size;
  }

  void SetHttpMaxPayloadSize(uint32_t max_payload_size) {
    options_.http_max_payload_size = max_payload_size;
  }
  std::string GeneratorKey(const std::string& source_key);
 protected:
  Options options_;

  // brpc channel related.
  std::string peer_host_;
  std::shared_ptr<brpc::Channel> channel_;

  // WaitAsyncSendToFinish
//   bthread::ConditionVariable wait_async_cv_;
  bthread::Mutex send_mutex_;
  int64_t running_async_count_ = 0;

  RpcKeyAdpapter rpc_key_adpapter_;
  RpcSeqSender rpc_seq_sender_;
};

class RpcGatewaySender : public RpcSender
{
public:
    const std::string GATEWAY_PROCESSOR = "proxyProcessor"; 
    RpcGatewaySender(Options options,const RpcKeyAdpapter& adpapter,GatewayPrama parma)
    :RpcSender(options,adpapter)
    ,gateway_parma_(parma)
    {}
private:
  void SendImpl(const std::string& key, const std::string& message,uint32_t timeout) override;
  // void SendChunked(const std::string& key, ByteContainerView value);
  void SendChunked(const std::string& key, const std::string& value);
private:
    GatewayPrama gateway_parma_;
    friend class RpcSenderTest;
};

} // namespace rpc
} // namespace mpc