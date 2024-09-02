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

#pragma once
#include "pir_data_handler.h"


namespace mpc{
namespace pir{

class PirDataHandlerFactory
{
public:
    static std::shared_ptr<PirDataHandlerBase> CreatePirDataHandler(const GlobalConfig& global_config,const std::string& key,const PirSetupRequest* request,const RemoveCallback& remove_callback);
};

}
}