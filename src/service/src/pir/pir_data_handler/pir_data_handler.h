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
#include <string>
#include <chrono>
#include <mutex>
#include <functional>
#include "src/config/config.h"
#include "src/pir/pir_service.pb.h"
#include "src/pir/pir_utils.h"
#include "spdlog/spdlog.h"
#include "brpc/channel.h"

namespace mpc{
namespace pir{

using RemoveCallback=std::function< void(const std::string&) > ;
struct PirSetupParma
{
    std::string version;
    std::string task_id;
    std::string data_file;
    std::string algorithm;
    std::vector<std::string> fields;
    std::vector<std::string> labels;
    std::string callbackUrl;
};

class PirDataHandlerBase
{
public:
    PirDataHandlerBase(const GlobalConfig& global_config,const std::string& key,const RemoveCallback& remove_callback)
    :global_config_(global_config),key_(key),is_setuped_(false),remove_callback_(remove_callback)
    {}
    virtual ~PirDataHandlerBase() = default;
    bool CheckParma(const PirSetupRequest* request, PirSetupResponse* response);
    void Setup() noexcept;
    void ResultCallback();

    virtual void SetupImpl() = 0;
    virtual PirType Type () = 0;

    bool IsSetup() const {return is_setuped_;}
    void SetupSucceed() {is_setuped_ = true;}
    void SetStatus(uint32_t status) { status_ = status;}
    void SetErrorMessage(const std::string& error_message) {error_message_ = error_message;} 
    std::string GeneratorInputPath(const std::string& filename)
    {
        return GetGlobalConfig().data_config_.source_data_path_ + "/"+filename;
    }

protected:
    const GlobalConfig& GetGlobalConfig() const {return global_config_;}
    std::string GetKey() const {return key_;}
protected:
    const GlobalConfig& global_config_;
    std::string key_;
    std::mutex setup_mutex_;
    bool is_setuped_;
    std::string task_id_;
    PirSetupParma parma_;
    brpc::Channel callback_channel_;
    uint32_t status_ = 200;
    std::string error_message_;
    uint32_t time_spend_;
    RemoveCallback remove_callback_;
};

}
}