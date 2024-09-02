// Copyright 2019 Ant Group Co., Ltd.
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
#include <map>
#include <mutex>
#include "src/pir/pir_service.pb.h"
#include "src/config/config.h"
#include "src/pir/pir_server/pir_server.h"
#include "src/pir/pir_client/pir_client.h"
namespace mpc{
namespace pir{
// class PirServer;
// class PirClient;
class PirDataHandlerBase;
class PirManager
{
public:
    static PirManager& GetInstance() {
        static PirManager instance(GetGlobalConfig());
        return instance;
    }

    ~PirManager()
    {
        Release();
    }

    void Setup(const PirSetupRequest* request,PirSetupResponse* reponse);
    void Remove(const PirRemoveRequest* request,PirRemoveResponse* response);

    // void QueryServer(const PirServerRequest* request,PirServerResponse* response);

    // void QueryClient(const PirClientRequest* request,PirClientResponse* response);
    std::shared_ptr<PirDataHandlerBase>  GetSetuper(const PirSetupRequest* request,PirSetupResponse* response);
    std::unique_ptr<PirServer> GetServer(const PirServerRequest* request,PirServerResponse* response);
    std::unique_ptr<PirClient> GetClient(const PirClientRequest* request,PirClientResponse* response);


private:
    PirManager(const GlobalConfig& global_config)
    :global_config_(global_config)
    {}

    std::string GeneratorKey(const PirSetupRequest* request);
    std::shared_ptr<PirDataHandlerBase> GetDataHandler(const std::string& key);
    std::shared_ptr<PirDataHandlerBase> CreateDataHandler(const std::string& key,const PirSetupRequest* request,bool& exit);
    void RemoveDataHandler(const std::string& key);
    void SetupRemoveCallback(const std::string& key);
    void Release();
private:
    const GlobalConfig& global_config_;
    std::map<std::string,std::shared_ptr<PirDataHandlerBase>> pir_data_handlers_;
    std::mutex  mutex_;
};

}
}