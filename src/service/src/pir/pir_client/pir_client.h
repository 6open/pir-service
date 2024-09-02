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
#include <memory>
#include <chrono>
#include "src/pir/pir_service.pb.h"
#include "src/config/config.h"
#include "spdlog/spdlog.h"

// #include "src/util/time_statistics.h"
namespace mpc{
namespace pir{

struct PirClientParma
{
    std::string version;
    std::string task_id;
    uint32_t rank;
    std::vector<std::string> members;
    std::vector<std::string> ips;
    std::vector<std::string> fields;
    std::vector<std::string> labels;
    std::string callbackUrl;
    std::string key;
    bool testLocal;
    std::string data_file;
    std::string query_ids;
};

class PirClient
{
public:
    PirClient(const GlobalConfig& global_config,const PirClientRequest* request,PirClientResponse* response);
    virtual ~PirClient() = default;
    bool CheckParma();
    bool TestLocal() const
    {
        return parma_.testLocal;
    }
    void RunService() noexcept
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        RunServiceImpl();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        time_spend_ = time_diff.count();
        SPDLOG_INFO(" time cost:{} milliseconds,taskid:{}",time_spend_,parma_.task_id);
        ResultCallback();
    }
    virtual void RunServiceImpl() = 0;
    virtual std::string CreateParties() = 0;
    std::string CreatePeerHost();

    void ResultCallback();
    std::string GeneratorInputPath(const std::string& filename)
    {
        return GetGlobalConfig().data_config_.source_data_path_ + "/"+filename;
    }

    std::string GeneratorOutputName(const std::string& task_id)
    {
        return task_id+"_pir_result.csv";
    }
    std::string GeneratorOutputPath(const std::string& filename)
    {
        return GetGlobalConfig().data_config_.output_data_path_+"/"+filename;
    }

    void GenOutputInfo(const std::string& task_id)
    {
        output_name_ = task_id+"_pir_result.txt";
        part_output_name_ = task_id+"_pir_result_part.txt";
        output_path_ = GetGlobalConfig().data_config_.output_data_path_+"/"+output_name_;
        part_output_path_ = GetGlobalConfig().data_config_.output_data_path_+"/"+part_output_name_;
    }
    std::string SelfMemberId()
    {
        return parma_.members[parma_.rank];
    }
    std::string PeerMemberId()
    {
        return parma_.members[1-parma_.rank];
    }
    int GetResult(google::protobuf::Map<std::string, PirDataValue>* result);
protected:
    const GlobalConfig& global_config_;
    const PirClientRequest* request_;
    PirClientResponse* response_;
    std::string parties_;
    PirClientParma parma_;
    std::string output_name_;
    std::string part_output_name_;
    std::string output_path_;
    std::string part_output_path_;
    uint32_t status_ = 200;
    std::string error_message_;
    uint32_t time_spend_;
    std::string server_profile_;
    std::string client_profile_;
    std::string result_data_;
};

std::string checkAndRestoreExperiment(const std::string &mlflow_addr, const std::string &experiment_name);

bool uploadToMlflow(const std::string& mlflow_addr, const std::string& experiment_id,
                               const std::string& run_name, const std::string& file_path, std::string& file_url);

}
}