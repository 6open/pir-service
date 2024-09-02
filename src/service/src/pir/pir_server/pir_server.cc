
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

#include "pir_server.h"
#include "src/http_server/api_service/utils.h"

namespace mpc{
namespace pir{

PirServer::PirServer(const GlobalConfig& global_config,std::shared_ptr<PirDataHandlerBase>& data_handler,const PirServerRequest* request,PirServerResponse* response)
    :global_config_(global_config)
    ,pir_data_handler_(data_handler)
    ,request_(request)
    ,response_(response)
{

}

bool PirServer::CheckParma()
{
    SPDLOG_INFO("CheckParma begin this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
    parma_.version = request_->version();
    parma_.task_id = request_->task_id();
    parma_.key = request_->key();

    parma_.rank = request_->rank();
    if(request_->members_size() == 0){
        parma_.members.push_back("member_self");
        parma_.members.push_back("member_peer");
    }else{
        for(int i = 0;i<request_->members_size();++i)
            parma_.members.push_back(request_->members(i));
    }

    if(request_->ips_size() == 0){
        parma_.ips.push_back("127.0.0.1");
        parma_.ips.push_back("127.0.0.1");
    }else{
        for(int i = 0;i<request_->ips_size();++i)
        parma_.ips.push_back(request_->ips(i));
    }
    parma_.testLocal = request_->testlocal();
    //parma_.ips[parma_.rank] = "0.0.0.0";
    parties_ = CreateParties();
    response_->set_task_id(parma_.task_id);
    response_->set_status(200);
    return true;
}

std::string PirServer::CreatePeerHost()
{
    if(parma_.testLocal)
    {
        return "127.0.0.1:"+std::to_string(global_config_.grpc_config_.other_port_);
    }
    if(global_config_.proxy_config_.proxy_mode_ == 1)
    {
        return parma_.ips[parma_.rank]+":"+std::to_string(global_config_.proxy_config_.gateway_config_.port_);
    }
    else
    {
        return parma_.ips[1-parma_.rank]+":"+std::to_string(global_config_.grpc_config_.other_port_);
    }
}

}
}