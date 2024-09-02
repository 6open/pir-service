
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
#include <iostream>
#include <fstream>
// #include <json/json.h>
#include "src/util/json.h"
#include "pir_client.h"
#include "brpc/channel.h"

namespace mpc{
namespace pir{

PirClient::PirClient(const GlobalConfig& global_config,const PirClientRequest* request,PirClientResponse* response)
    :global_config_(global_config)
    ,request_(request)
    ,response_(response)
{

}

bool PirClient::CheckParma()
{
    parma_.version = request_->version();
    parma_.task_id = request_->task_id();
    parma_.data_file = request_->data_file();
    parma_.query_ids = request_->query_ids();
    parma_.key = request_->key();

    parma_.rank = request_->rank();
    if(request_->members_size() == 0){
        parma_.members.push_back("member_self");
        parma_.members.push_back("member_peer");
    }else{
        for(int i = 0;i<request_->members_size();++i)
            parma_.members.push_back(request_->members(i));
    }
    if(request_->ips_size() == 0){
        parma_.ips.push_back("127.0.0.1");
        parma_.ips.push_back("127.0.0.1");
    }else{
        for(int i = 0;i<request_->ips_size();++i)
        parma_.ips.push_back(request_->ips(i));
    }

    for(int i = 0;i<request_->fields_size();++i)
        parma_.fields.push_back(request_->fields(i));
    for(int i = 0;i<request_->labels_size();++i)
        parma_.labels.push_back(request_->labels(i));
    parma_.testLocal = request_->testlocal();
    parma_.callbackUrl = request_->callback_url();
    if(parma_.testLocal && parma_.callbackUrl.empty()) {
        parma_.callbackUrl = "http://127.0.0.1:"+std::to_string(GetGlobalConfig().http_config_.port_) + "/v1/service/pir/client_callback";
    }
    
    //parma_.ips[parma_.rank] = "0.0.0.0";
    parties_ = CreateParties();
    // output_name_ = GeneratorOutputName(parma_.task_id);
    // output_path_ = GeneratorOutputPath(output_name_);

    GenOutputInfo(parma_.task_id);

    SPDLOG_INFO("taskid:{} data:{} rank:{} key:{} members_size:{}  ips_size:{} fields_size:{} labels_size:{} parties_:{} output_name_:{} output_path_:{} callbackUrl:{}"
               ,parma_.task_id,parma_.data_file,parma_.rank,parma_.key
               ,parma_.members.size(),parma_.ips.size(),parma_.fields.size(),parma_.labels.size()
               ,parties_,output_name_,output_path_,parma_.callbackUrl);
    response_->set_task_id(parma_.task_id);
    response_->set_status(200);
    return true;
}

void uploadResult(
        const std::string& full_path, const std::string full_file_name,
        const std::string& part_path, const std::string part_file_name,
        std::string& full_url, std::string& part_url) {
    
    std::string experiment_name = "mpc-pir-1k";
    std::string run_name = "task_1";
    try {
        std::string mlflow_addr;
        const char* mlflow_uri_cstr = std::getenv("MLFLOW_TRACKING_URI");
        mlflow_addr = mlflow_uri_cstr ? mlflow_uri_cstr : "";
        if (mlflow_addr.empty()) {
            SPDLOG_ERROR("MLFLOW_TRACKING_URI environment variable is not set.");
            full_url.assign("MLFLOW_TRACKING_URI not set");
            part_url.assign("MLFLOW_TRACKING_URI not set");
            return ;
        }

        std::string experiment_id = checkAndRestoreExperiment(mlflow_addr, experiment_name);
        if (experiment_id.empty()) {
            std::cerr << "Failed to restore or create experiment." << std::endl;
            return ;
        }

        if (uploadToMlflow(mlflow_addr, experiment_id, full_file_name, full_path, full_url)) {
            SPDLOG_INFO("Upload full file succesful : {}", full_file_name);
        } else {
            SPDLOG_INFO("Upload full file failed, {}", full_path);
        }

        if (uploadToMlflow(mlflow_addr, experiment_id, part_file_name, part_path, part_url)) {
            SPDLOG_INFO("Upload part file succesful : {}", part_file_name);
        } else {
            SPDLOG_INFO("Upload part file failed, {}", part_path);
        }
    }
    catch (const std::exception& e){
        SPDLOG_ERROR("Exception caught: {}", e.what());
        full_url.assign("MLFLOW_TRACKING_URI not set");
        part_url.assign("MLFLOW_TRACKING_URI not set");
        return ;
    }


}

void PirClient::ResultCallback()
{
    SPDLOG_INFO("begin callback status_:{}, taskid:{}, url: {}",status_,parma_.task_id, parma_.callbackUrl.c_str());
    brpc::Channel channel;
    brpc::ChannelOptions options;
    options.protocol = brpc::PROTOCOL_HTTP;
    if (channel.Init(parma_.callbackUrl.c_str(), &options) != 0)
    {
        SPDLOG_ERROR("Fail to initialize channel");
        return;
    }
    brpc::Controller cntl;
    PirCallbackRequest callback;
    PirCallbackResponse response;
    cntl.http_request().uri() = parma_.callbackUrl;  // 设置为待访问的URL
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    callback.set_status(status_);
    callback.set_message(error_message_);
    callback.set_task_id(parma_.task_id);
    callback.set_service_type("PIR");
    google::protobuf::RepeatedPtrField<PirDataContent>* dataResult = callback.mutable_data_result();
    PirDataContent* dataContent = dataResult->Add();
    google::protobuf::Map<std::string, PirDataValue>* data_map = dataContent->mutable_data_content();
    std::ifstream file(part_output_path_);
    if (!file.is_open()) {
        SPDLOG_ERROR("Failed to open temporary file for reading.");
        return ;
    }
    
    std::string line;
    std::getline(file, line); //跳过首行
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string key;
        json values;
        PirDataValue dataValue;

        if (std::getline(iss, key, ',')) {
            std::string element;
            while (std::getline(iss, element, ',')) {
                dataValue.add_values(element);
            }

        } else {
            std::cerr << "Invalid input line: " << line << std::endl;
        }

        (*data_map)[key] = dataValue;
    }
    file.close();

    std::string full_url, part_url;
    uploadResult(output_path_, output_name_, part_output_path_, output_name_, full_url, part_url);
    callback.set_full_result_file(full_url);
    callback.set_part_result_file(part_url);

    // callback.set_server_profile(server_profile_);
    // callback.set_client_profile(client_profile_);
    std::string body;
    SPDLOG_INFO("callback status_:{}, timespend:{}, body:{} taskid:{}",status_, time_spend_, callback.DebugString(),parma_.task_id);
    channel.CallMethod(NULL, &cntl, &callback, NULL, NULL/*done*/);
    if (cntl.Failed()) 
    {
        SPDLOG_INFO("taskid:{} error{} ",parma_.task_id,cntl.ErrorText());
    }
}



std::string PirClient::CreatePeerHost()
{
    if(parma_.testLocal)
    {
        return "127.0.0.1:"+std::to_string(global_config_.grpc_config_.self_port_);
    }
    if(global_config_.proxy_config_.proxy_mode_ == 1)
    {
        return parma_.ips[parma_.rank]+":"+std::to_string(global_config_.proxy_config_.gateway_config_.port_);
    }
    else
    {
        return parma_.ips[1-parma_.rank]+":"+std::to_string(global_config_.grpc_config_.other_port_);
    }
}

}
}