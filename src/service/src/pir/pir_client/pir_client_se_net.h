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

#pragma once

#include "pir_client.h"
// #include "src/util/mock_spu_utils.h"

namespace mpc{
namespace pir{
class PirClientSeNet : public PirClient 
{
public:
    PirClientSeNet(const GlobalConfig& global_config,const PirClientRequest* request,PirClientResponse* response)
    :PirClient(global_config,request,response)
    {}
    virtual ~PirClientSeNet();
    void RunServiceImpl() override;
    std::string CreateParties() override;
    // std::unique_ptr<spu::HalContext> MakeHalContext();
    void SaveResult(const std::vector<std::string>& fileds,const std::vector<std::string>& labels);

};

}
}