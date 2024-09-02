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


#include "pir_data_handler_se.h"
#include "yacl/io/rw/csv_reader.h"
#include "yacl/io/stream/file_io.h"
#include "src/pir/pir_fse_prama.h"
#include "src/pir/pir_utils.h"
#include "src/kernel/data_db/db_meta.h"

namespace mpc{
namespace pir{

PirDataHandlerSe::PirDataHandlerSe(const GlobalConfig& global_config,const std::string& key,const RemoveCallback& remove_callback)
:PirDataHandlerBase(global_config,key,remove_callback)
{
    SPDLOG_INFO("PirDataHandlerSe::PirDataHandlerSe this {} key:{}",fmt::ptr(static_cast<void*>(this)),key);
}
PirDataHandlerSe::~PirDataHandlerSe()
{
    SPDLOG_INFO("PirDataHandlerSe::~PirDataHandlerSe this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),task_id_);
    // server_wrapper_.db_free();
}

void PirDataHandlerSe::SetupImpl()
{
    try{    
        // std::
        std::string data_path = GeneratorInputPath(parma_.data_file);
        // std::string meta_path = GetGlobalConfig().pir_config_.data_meta_path;
        DbMeta*  db_meta = DbMetaManager::Instance().GetDbMeta(GetKey(),data_path,parma_.fields,parma_.labels);
        db_meta->Setup();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("task_id:{}, error:{}", parma_.task_id,e.what());
        SetStatus(201);
        SetErrorMessage(e.what());
    }
}

std::string PirDataHandlerSe::HandleOprfRequest(const std::string& oprf_request)
{
    return server_wrapper_.handle_oprf_request(oprf_request);
}

std::string PirDataHandlerSe::HandleQuery(const std::string& query_string)
{
    return server_wrapper_.handle_query(query_string);
}



}
}