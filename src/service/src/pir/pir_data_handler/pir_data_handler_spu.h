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

#include "pir_data_handler.h"

namespace mpc{
namespace pir{

class PirDataHandlerSpu : public PirDataHandlerBase
{
public:
    PirDataHandlerSpu(const GlobalConfig& global_config,const std::string& key,const RemoveCallback& remove_callback)
    :PirDataHandlerBase(global_config,key,remove_callback)
    {}
    virtual ~PirDataHandlerSpu()
    {
        RemoveSetupDir(setup_path_);
    }
    PirType Type () override
    {
        return PirType::SPU;
    }
    void SetupImpl() override;

    const std::string&  OprfPath() {return oprf_path_;}
    const std::string& SetupPath() {return setup_path_;}
private:
    std::string GeneratorSetupDir(const std::string& key);
    void RemoveSetupDir(const std::string& dir_path);
    void GeneratorOprfFile(const std::string& key);
private:
    // spu::pir::PirSetupConfig config_;
    std::string oprf_path_;
    std::string setup_path_;
    // std::mutex mutex_;
};

}
}