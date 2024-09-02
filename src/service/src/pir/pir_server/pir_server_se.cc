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

#include "pir_server_se.h"
#include "src/pir/pir_data_handler/pir_data_handler_se.h"
#include "src/pir/pir_utils.h"
#include "src/util/mock_spu_utils.h"

namespace mpc{
namespace pir{


PirServerSe::PirServerSe(const GlobalConfig& global_config,std::shared_ptr<PirDataHandlerBase>& data_handler,const PirServerRequest* request,PirServerResponse* response)
:PirServer(global_config,data_handler,request,response)
{
    SPDLOG_INFO("PirServerSe::PirServerSe this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),request->task_id());
}

PirServerSe::~PirServerSe()
{
    SPDLOG_INFO("PirServerSe::~PirServerSe this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
}

void PirServerSe::RunServiceImpl()
{
    try {

        SPDLOG_INFO("RunServiceImpl begin this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
        auto hctx = MakeHalContext();
        auto link_ctx = hctx->lctx();

        link_ctx->SetRecvTimeout(kLinkRecvTimeout);
        std::shared_ptr<PirDataHandlerSe> dh = std::dynamic_pointer_cast<PirDataHandlerSe>(DataHandler());
    
        yacl::Buffer oprf_request_buffer = link_ctx->Recv(link_ctx->NextRank(), fmt::format("recv oprf_request"));
        // std::string_view oprf_request(oprf_request_buffer);
        std::string oprf_request{std::string_view(oprf_request_buffer)};

        SPDLOG_INFO(" recv oprf_request size:{} task_id:{}",oprf_request.size(),parma_.task_id);
    
        std::string oprf_response = dh->HandleOprfRequest(oprf_request);
        SPDLOG_INFO(" send oprf_response size:{} task_id:{}",oprf_response.size(),parma_.task_id);
        yacl::Buffer oprf_response_buffer(oprf_response.data(),oprf_response.size());
        link_ctx->SendAsync(link_ctx->NextRank(), oprf_response_buffer,fmt::format("send oprf_response"));

        yacl::Buffer query_string_buffer = link_ctx->Recv(link_ctx->NextRank(), fmt::format("recv query_string"));
        // std::string_view query_string(query_string_buffer);
        std::string query_string{std::string_view(query_string_buffer)};
        SPDLOG_INFO(" recv query_string size:{} task_id:{}",query_string.size(),parma_.task_id);

        std::string response_string = dh->HandleQuery(query_string);
        SPDLOG_INFO(" send response_string size:{} task_id:{}",response_string.size(),parma_.task_id);
        yacl::Buffer response_string_buffer(response_string.data(),response_string.size());
        link_ctx->SendAsync(link_ctx->NextRank(), response_string_buffer,fmt::format("send response_string"));


    } catch (const std::exception& e) {
        SPDLOG_ERROR("run pir failed: {} taskid:{}", e.what(),parma_.task_id);
    }

} 

std::string PirServerSe::CreateParties()
{
    return  mpc::utils_fse::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
}

std::unique_ptr<spu::HalContext> PirServerSe::MakeHalContext()
{
    return mpc::utils_fse::MakeHalContext(parties_,parma_.rank,global_config_,parma_.members);
}

}
}