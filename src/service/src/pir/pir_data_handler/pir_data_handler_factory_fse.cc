// Copyright 2019 Ant Group Co., Ltd
//PirDataHandlerFactory
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

#include "pir_data_handler_factory_fse.h"
#include "pir_data_handler_se.h"
#include "src/pir/pir_utils.h"

namespace mpc{
namespace pir{


std::shared_ptr<PirDataHandlerBase> PirDataHandlerFactory::CreatePirDataHandler(const GlobalConfig& global_config,const std::string& key,const PirSetupRequest* request,const RemoveCallback& remove_callback)
{
    std::string_view algo = request->algorithm();
    PirType type = GetPirType(algo);
    switch(type)
    {
        case PirType::SE:
            return std::make_shared<PirDataHandlerSe>(global_config,key,remove_callback);
        default:
            return nullptr;
    }
}

}
}