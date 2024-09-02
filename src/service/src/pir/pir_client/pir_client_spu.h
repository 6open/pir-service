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
#include "src/pir/pir_utils.h"
#include "src/http_server/api_service/utils.h"


namespace mpc{
namespace pir{
class PirClientSpu : public PirClient 
{
public:
    PirClientSpu(const GlobalConfig& global_config,const PirClientRequest* request,PirClientResponse* response)
    :PirClient(global_config,request,response)
    {}
    virtual ~PirClientSpu() = default;
    void RunServiceImpl() override
    {
        try {
            auto hctx = MakeHalContext();
            auto link_ctx = hctx->lctx();

            link_ctx->SetRecvTimeout(kLinkRecvTimeout);

            spu::pir::PirClientConfig config;

            config.set_pir_protocol(spu::pir::PirProtocol::KEYWORD_PIR_LABELED_PSI);

            config.set_input_path("/home/admin/data/"+parma_.data);
            config.mutable_key_columns()->Add(parma_.fields.begin(), parma_.fields.end());
            output_name_ = "/home/admin/data/haha.csv";
            config.set_output_path(output_name_);

            spu::pir::PirResultReport report = spu::pir::PirClient(link_ctx, config);

            SPDLOG_INFO("data count:{}", report.data_count());
         } catch (const std::exception& e) {
            SPDLOG_ERROR("run PirClientSpu failed: {} taskid:{}", e.what(),parma_.task_id);
            status_ = 201;
            error_message_ = e.what();
         }

    }   

    std::string CreateParties() override
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