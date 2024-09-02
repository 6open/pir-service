
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
#include "src/util/mock_spu_utils.h"

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
    parma_.version = request_->version();
    parma_.task_id = request_->task_id();

    parma_.rank = request_->rank();
    for(int i = 0;i<request_->members_size();++i)
        parma_.members.push_back(request_->members(i));
    for(int i = 0;i<request_->ips_size();++i)
        parma_.ips.push_back(request_->ips(i));
    parma_.testLocal = request_->testlocal();
    //parma_.ips[parma_.rank] = "0.0.0.0";
    parties_ = mpc::utils_fse::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
    response_->set_task_id(parma_.task_id);
    response_->set_status(200);
    return true;
}


}
}