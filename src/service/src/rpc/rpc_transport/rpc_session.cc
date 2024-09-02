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


#include "rpc_session.h"
#include "spdlog/spdlog.h"

namespace mpc {
namespace rpc {

RpcSession::RpcSession(RpcServer* rpc_server,const std::string& peer_host,GatewayPrama parma)
:rpc_server_(rpc_server)
,peer_host_(peer_host)
,parma_(parma)
,sender_key_(parma.self_memberid+"_"+parma.peer_memberid+"_"+parma.task_id)
,receiver_key_(parma.peer_memberid+"_"+parma.self_memberid+"_"+parma.task_id)
,rpc_key_sender_adpapter_(sender_key_)
,rpc_key_receiver_adpapter_(receiver_key_)
{
    SPDLOG_INFO("RpcSession::RpcSession peer_host:{},sender_key_:{} receiver_key_:{} this:{}",peer_host_,sender_key_,receiver_key_,fmt::ptr(static_cast<void*>(this)));
    CreateSender();
    CreateReceive();
}

RpcSession::~RpcSession()
{
    SPDLOG_INFO("RpcSession::~RpcSession peer_host:{},sender_key_:{} receiver_key_:{} this:{}",peer_host_,sender_key_,receiver_key_,fmt::ptr(static_cast<void*>(this)));
    rpc_server_->RemoveReceiver(rpc_key_receiver_adpapter_.Key());
}

std::string RpcSession::BuildSessionKey(const std::string& self_memberid,const std::string& peer_memberid,const std::string& taskid)
{
    return self_memberid+"_"+peer_memberid+"_"+taskid;
}
void RpcSession::CreateSenderKey()
{
    // sender_key_ = self_memberid+"_"+peer_memberid+"_"+taskid;
}

void RpcSession::CreateReceiverKey()
{
    // receiver_key_ = peer_memberid+"_"+self_memberid+"_"+taskid;
}

void RpcSession::CreateSender()
{
    CreateSenderKey();
    RpcSender::Options option;
    // option.channel_protocol = "h2:grpc";
    rpc_sender = std::make_unique<RpcGatewaySender>(option,rpc_key_sender_adpapter_,parma_);
    rpc_sender->SetPeerHost(peer_host_);
}

void RpcSession::CreateReceive()
{
    CreateReceiverKey();
    rpc_receiver_ = rpc_server_->AddReceiver(rpc_key_receiver_adpapter_.Key());
}

std::string RpcSession::Recv(const std::string& msg_key)
{
    return rpc_receiver_->Recv(msg_key);
}

void RpcSession::Send(const std::string& msg_key,const std::string& value)
{
    rpc_sender->Send(msg_key,value);
}


std::unique_ptr<RpcSession> RpcSessionFactory::CreateRpcSession(const std::string& peer_host,GatewayPrama parma)
{
    return std::make_unique<RpcSession>(&RpcServer::GetInstance(),peer_host,parma);
}

} // namespace rpc
} // namespace mpc