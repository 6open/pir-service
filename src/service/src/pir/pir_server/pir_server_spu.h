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

#include "libspu/pir/pir.h"
#include "pir_server.h"
#include "src/pir/pir_data_handler/pir_data_handler_spu.h"
namespace mpc{
namespace pir{

class PirServerSpu : public PirServer 
{
public:
    PirServerSpu(const GlobalConfig& global_config,std::shared_ptr<PirDataHandlerBase>& data_handler,const PirServerRequest* request,PirServerResponse* response)
    :PirServer(global_config,data_handler,request,response)
    {}
    virtual ~PirServerSpu() = default;
    void RunServiceImpl() override
    {
        try {

            auto hctx = MakeHalContext();
            auto link_ctx = hctx->lctx();

            link_ctx->SetRecvTimeout(kLinkRecvTimeout);

            spu::pir::PirServerConfig config;

            config.set_pir_protocol(spu::pir::PirProtocol::KEYWORD_PIR_LABELED_PSI);
            config.set_store_type(spu::pir::KvStoreType::LEVELDB_KV_STORE);

            std::shared_ptr<PirDataHandlerSpu> dh_spu = std::dynamic_pointer_cast<PirDataHandlerSpu>(DataHandler());
            config.set_oprf_key_path(dh_spu->OprfPath());
            config.set_setup_path(dh_spu->SetupPath());

            spu::pir::PirResultReport report = spu::pir::PirServer(link_ctx, config);
         } catch (const std::exception& e) {
            SPDLOG_ERROR("run pir failed: {} taskid:{}", e.what(),parma_.task_id);
         }

    }   

    std::string CreateParties()
    {
        return  mpc::utils::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
    }

    std::unique_ptr<spu::HalContext> MakeHalContext()
    {
        return mpc::utils::MakeHalContext(parties_,parma_.rank,global_config_,parma_.members);
    }
};

}
}