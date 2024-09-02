// Copyright 2019 Ant Group Co., Ltd
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

#include "pir_server_se_net.h"
#include "src/pir/pir_data_handler/pir_data_handler_se.h"
#include "src/pir/pir_utils.h"
#include "src/rpc/rpc_transport/rpc_session.h"
#include "src/kernel/data_db/db_server.h"
// #include "src/util/mock_spu_utils.h"

namespace mpc{
namespace pir{


PirServerSeNet::PirServerSeNet(const GlobalConfig& global_config,std::shared_ptr<PirDataHandlerBase>& data_handler,const PirServerRequest* request,PirServerResponse* response)
:PirServer(global_config,data_handler,request,response)
{
    SPDLOG_INFO("PirServerSeNet::PirServerSeNet this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),request->task_id());
}

PirServerSeNet::~PirServerSeNet()
{
    SPDLOG_INFO("PirServerSeNet::~PirServerSeNet this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
}

void PirServerSeNet::RunServiceImpl()
{
    try {

        SPDLOG_INFO("RunServiceImpl begin this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
        mpc::rpc::GatewayPrama gateway_prama;
        gateway_prama.self_memberid = SelfMemberId();
        gateway_prama.peer_memberid = PeerMemberId();
        gateway_prama.session_id = parma_.task_id;
        gateway_prama.task_id = parma_.task_id;
        std::string host_str = CreatePeerHost();
        SPDLOG_INFO("host_str : {}", host_str);
        auto rpc_session = mpc::rpc::RpcSessionFactory::CreateRpcSession(host_str,gateway_prama);

        rpc_session->SetRecvTimeout(kLinkRecvTimeout);
        auto server_sender = [&rpc_session](const std::string& message_key,const std::string& value)
        {
            return rpc_session->Send(message_key,value);
        };
        auto server_recv = [&rpc_session](const std::string& message_key)
        {
            return rpc_session->Recv(message_key);
        };
        DbServer server(parma_.key,parma_.task_id,server_sender,server_recv);
        server.Run();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("run pir failed: {} taskid:{}", e.what(),parma_.task_id);
    }

} 

std::string PirServerSeNet::CreateParties()
{
    // return  mpc::utils_fse::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
    return "";
}

// std::unique_ptr<spu::HalContext> PirServerSe::MakeHalContext()
// {
//     return mpc::utils_fse::MakeHalContext(parties_,parma_.rank,global_config_,parma_.members);
// }

}
}