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
#include "src/pir/pir_data_handler/pir_data_handler.h"
#include "spdlog/spdlog.h"

namespace spu {
    class HalContext;
};
namespace mpc{
namespace pir{

struct PirServerParma
{
    std::string version;
    std::string task_id;
    std::string key;
    uint32_t rank;
    std::vector<std::string> members;
    std::vector<std::string> ips;
    bool testLocal;
};
class PirServer
{
public:
    PirServer(const GlobalConfig& global_config,std::shared_ptr<PirDataHandlerBase>& data_handler,const PirServerRequest* request,PirServerResponse* response);
    virtual ~PirServer() = default;
    bool CheckParma();
    bool TestLocal() const
    {
        return parma_.testLocal;
    }
    void RunService()
    {
        auto start_time = std::chrono::high_resolution_clock::now();
        RunServiceImpl();
        auto end_time = std::chrono::high_resolution_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        time_spend_ = time_diff.count();
        SPDLOG_INFO(" time cost:{} milliseconds,taskid:{}",time_spend_,parma_.task_id);
    }
    virtual void RunServiceImpl() = 0;
    virtual std::string CreateParties() = 0;
    std::string CreatePeerHost();
    
    std::shared_ptr<PirDataHandlerBase>& DataHandler() {return pir_data_handler_;}
    std::string SelfMemberId()
    {
        return parma_.members[parma_.rank];
    }
    std::string PeerMemberId()
    {
        return parma_.members[1-parma_.rank];
    }
protected:
    const GlobalConfig& global_config_;
    std::shared_ptr<PirDataHandlerBase> pir_data_handler_;
    const PirServerRequest* request_;
    PirServerResponse* response_;
    std::string parties_;
    PirServerParma parma_;
    uint32_t time_spend_;
};

}
}

