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


#include "pir_client_factory_fse.h"
#include "src/pir/pir_client/pir_client_se_net.h"

namespace mpc{
namespace pir{

std::unique_ptr<PirClient> PirClientFactory::CreatePirClient(const GlobalConfig& global_config,const PirClientRequest* request,PirClientResponse* response)
{
    std::string algo = request->algorithm();
    PirType type = GetPirType(algo);

    switch(type)
    {
        case PirType::SE:
            return std::make_unique<PirClientSeNet>(global_config,request,response);
        default:
            return nullptr;
    }
}

}
}