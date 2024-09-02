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


#include "pir_data_handler_spu.h"
#include <random>
#include <fstream>
#include <filesystem>
#include <iterator>
#include "libspu/pir/pir.h" 
#include "libspu/pir/pir.pb.h"
namespace mpc{
namespace pir{

void PirDataHandlerSpu::SetupImpl()
{
    std::lock_guard<std::mutex> lock(setup_mutex_);
    // response->set_status(200);
    if(IsSetup())
    {
        // response->set_key(GetKey());
        return;
    }
    try{
        std::string input_path = GeneratorInputPath(parma_.data);
        GeneratorOprfFile(GetKey());
        GeneratorSetupDir(GetKey());
        spu::pir::PirSetupConfig config;

        config.set_pir_protocol(spu::pir::PirProtocol::KEYWORD_PIR_LABELED_PSI);
        config.set_store_type(spu::pir::KvStoreType::LEVELDB_KV_STORE);
        config.set_input_path(input_path);
        for(std::size_t i=0; i<parma_.fields.size(); ++i)
            config.add_key_columns(parma_.fields[i]);
        for(std::size_t i=0; i<parma_.labels.size(); ++i)
            config.add_label_columns(parma_.labels[i]);

        config.set_num_per_query(GetGlobalConfig().pir_config_.count_per_query_);
        auto label_max_len = 5+GetGlobalConfig().pir_config_.max_label_length_*parma_.labels.size();
        config.set_label_max_len(label_max_len);
        config.set_oprf_key_path(oprf_path_);
        config.set_setup_path(setup_path_);
        SPDLOG_INFO("input_path:{}, oprf:{},setup:{}", input_path,oprf_path_,setup_path_);
        spu::pir::PirResultReport report = spu::pir::PirSetup(config);
        SPDLOG_INFO("data count:{}", report.data_count());
        // response->set_key(GetKey());
        SetupSucceed();
    } catch (const std::exception& e)
    {
        SPDLOG_ERROR("task_id:{}, error:{}", parma_.task_id,e.what());
        SetStatus(201);
        SetErrorMessage(e.what());
    }
}

std::string PirDataHandlerSpu::GeneratorSetupDir(const std::string& key)
{

    std::string path = GetGlobalConfig().pir_config_.apsi_setup_path_+"/"+key;
    //remove if exit
    RemoveSetupDir(path);
    RemoveSetupDir(setup_path_);
    std::filesystem::create_directories(path);
    setup_path_ = path;
    return path;
}

void PirDataHandlerSpu::RemoveSetupDir(const std::string& dir_path)
{
    if(!dir_path.empty())
        std::filesystem::remove_all(dir_path);
}

void PirDataHandlerSpu::GeneratorOprfFile(const std::string& key)
{
    std::filesystem::create_directories(GetGlobalConfig().pir_config_.oprf_key_path_);
    std::string path = GetGlobalConfig().pir_config_.oprf_key_path_;
    std::string filename = path + "/orpf_"+key+".bin";
    std::ofstream file(filename, std::ios::binary|std::ios::out);
    std::random_device rd;
    std::vector<uint8_t> random_data(32);
    std::generate_n(random_data.begin(), 32, std::ref(rd));
    file.write(reinterpret_cast<const char*>(random_data.data()), random_data.size());
    file.close();
    oprf_path_ = filename;
}

}
}