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

#include "pir_server_factory_fse.h"
#include "pir_server.h"
#include "src/pir/pir_data_handler/pir_data_handler.h"
// #include "pir_server_se.h"
#include "pir_server_se_net.h"
#include "src/pir/pir_utils.h"

namespace mpc{
namespace pir{

std::unique_ptr<PirServer> PirServerFactory::CreatePirServer(const GlobalConfig& global_config,std::shared_ptr<PirDataHandlerBase>& data_handler,const PirServerRequest* request,PirServerResponse* response)
{
    std::string_view algo = request->algorithm();
    PirType type1 = GetPirType(algo);
    //
    auto type2 = data_handler->Type();
    if(type1 != type2) {
        SPDLOG_ERROR("type1 {} != type2 {}");
        response->set_status(201);
        response->set_message("algorithm between in setup and server should same");
        return nullptr;
    }
    switch(type1)
    {
        case PirType::SE:
            return std::make_unique<PirServerSeNet>(global_config,data_handler,request,response);
        default:
            response->set_status(201);
            response->set_message("algorithm not supported");
            return nullptr;
    }
}

}
}