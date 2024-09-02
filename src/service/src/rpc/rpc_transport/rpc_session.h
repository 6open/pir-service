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
#include <functional>
#include "rpc_receiver.h"
#include "rpc_sender.h"
#include "rpc_server.h"
namespace mpc {
namespace rpc {
class RpcSession
{
public:
    RpcSession(RpcServer* rpc_server,const std::string& peer_host,GatewayPrama parma);
    ~RpcSession();

    static std::string BuildSessionKey(const std::string& self_memberid,const std::string& peer_memberid,const std::string& taskid);

    std::string Recv(const std::string& msg_key);
    void Send(const std::string& msg_key,const std::string& value);
    void SetRecvTimeout(uint32_t recv_timeout_ms) {rpc_receiver_->SetRecvTimeout(recv_timeout_ms);} 
private:
    void CreateReceive();
    void CreateSender();
    void CreateSenderKey();
    void CreateReceiverKey();

private:
    RpcServer* rpc_server_;
    std::string peer_host_;
    GatewayPrama parma_;
    std::string sender_key_;  
    std::string receiver_key_;
    RpcKeyAdpapter rpc_key_sender_adpapter_;
    RpcKeyAdpapter rpc_key_receiver_adpapter_;

    std::shared_ptr<RpcReceiver> rpc_receiver_;
    std::unique_ptr<RpcSender> rpc_sender;
};

class RpcSessionFactory
{
public:
    static std::unique_ptr<RpcSession> CreateRpcSession(const std::string& peer_host,GatewayPrama parma);
};
} // namespace rpc
} // namespace mpc