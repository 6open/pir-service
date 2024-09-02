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

#include "src/pir/pir_manager.h"
#include "src/pir/pir_client/pir_client_factory_fse.h"
#include "src/pir/pir_server/pir_server_factory_fse.h"
#include "src/pir/pir_data_handler/pir_data_handler_factory_fse.h"

namespace mpc{
namespace pir{

void PirManager::Setup(const PirSetupRequest* request,PirSetupResponse* response)
{
    // std::string key = GeneratorKey(request);
    // bool exit = false;
    // auto data_handler = CreateDataHandler(key,request,exit);
    // if(nullptr == data_handler){
    //     response->set_task_id( request->task_id());
    //     response->set_status(201);
    //     response->set_message(request->algorithm()+" algorithm not supported");
    //     RemoveDataHandler(key);
    //     return;
    // }
    // data_handler->Setup();
}

void PirManager::Remove(const PirRemoveRequest* request,PirRemoveResponse* response)
{
    std::string key = request->key();
    RemoveDataHandler(key);
}

std::shared_ptr<PirDataHandlerBase>  PirManager::GetSetuper(const PirSetupRequest* request,PirSetupResponse* response)
{
    std::string key = GeneratorKey(request);
    bool exit = false;
    return CreateDataHandler(key,request,exit);
}

std::unique_ptr<PirServer> PirManager::GetServer(const PirServerRequest* request,PirServerResponse* response)
{
    std::string key = request->key();
    auto data_handler = GetDataHandler(key);
    if(data_handler == nullptr){
        response->set_status(201);
        response->set_message("data not setuped");
        return nullptr;
    }
    return PirServerFactory::CreatePirServer(global_config_,data_handler,request,response);
}

std::unique_ptr<PirClient> PirManager::GetClient(const PirClientRequest* request,PirClientResponse* response)
{
    return PirClientFactory::CreatePirClient(global_config_,request,response);
}

std::string PirManager::GeneratorKey(const PirSetupRequest* request)
{
    std::string str = request->algorithm()+"_"+request->data_file();
    std::vector<std::string> ids;
    std::vector<std::string> labels;
    for(auto i=0; i<request->fields_size(); ++i)
        ids.push_back(request->fields(i));
    for(auto i=0; i<request->labels_size(); ++i)
        labels.push_back(request->labels(i));
    std::sort(ids.begin(),ids.end());
    std::sort(labels.begin(),labels.end());
    for(std::size_t i=0;i<ids.size();++i)
        str += "_fields:"+ids[i];
    for(std::size_t i=0;i<labels.size();++i)
        str += "_labels:"+labels[i]; 
    std::hash<std::string> hasher;
    std::size_t hash_value = hasher(str);
    SPDLOG_INFO("str:{} hash_value:{},  task_id:{}", str,hash_value,request->task_id());
    return std::to_string(hash_value);
}

std::shared_ptr<PirDataHandlerBase> PirManager::GetDataHandler(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = pir_data_handlers_.find(key);
    if(iter == pir_data_handlers_.end())
        return nullptr;
    return iter->second;
}

std::shared_ptr<PirDataHandlerBase> PirManager::CreateDataHandler(const std::string& key,const PirSetupRequest* request,bool& exit)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = pir_data_handlers_.find(key);
    if(iter != pir_data_handlers_.end())
    {
        SPDLOG_INFO("key:{} exit  task_id:{}", key,request->task_id());
        return iter->second;
    }
    else
    {
        auto callback = std::bind(&PirManager::SetupRemoveCallback, this, std::placeholders::_1);
        std::shared_ptr<PirDataHandlerBase> data_handler = PirDataHandlerFactory::CreatePirDataHandler(global_config_,key,request,callback);
        pir_data_handlers_.insert(std::make_pair(key,data_handler));
        return data_handler;
    }
}

void PirManager::SetupRemoveCallback(const std::string& key)
{
    SPDLOG_INFO("key:{}", key);
    RemoveDataHandler(key);
}

void PirManager::RemoveDataHandler(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    pir_data_handlers_.erase(key);
}

void PirManager::Release()
{
    std::lock_guard<std::mutex> lock(mutex_);
    pir_data_handlers_.clear();
}

}
}