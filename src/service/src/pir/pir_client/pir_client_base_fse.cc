
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

#include "pir_client.h"
#include "brpc/channel.h"
#include "src/util/mock_spu_utils.h"

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
    parma_.data = request_->data();

    parma_.rank = request_->rank();
    for(int i = 0;i<request_->members_size();++i)
        parma_.members.push_back(request_->members(i));
    for(int i = 0;i<request_->ips_size();++i)
        parma_.ips.push_back(request_->ips(i));
    for(int i = 0;i<request_->fields_size();++i)
        parma_.fields.push_back(request_->fields(i));
    for(int i = 0;i<request_->labels_size();++i)
        parma_.labels.push_back(request_->labels(i));
    for(int i = 0;i<request_->query_ids_size;++i)
        parma_.query_ids.push_back(request_->query_ids(i));
    parma_.testLocal = request_->testlocal();
    if(parma_.testLocal) {
        parma_.callbackUrl = "http://127.0.0.1:"+std::to_string(GetGlobalConfig().http_config_.port_) + "/v1/service/pir/client_callback";
    }
    else {
        parma_.callbackUrl = request_->callback_url();
    }
    //parma_.ips[parma_.rank] = "0.0.0.0";
    parties_ = mpc::utils_fse::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
    output_name_ = GeneratorOutputName(parma_.task_id);
    output_path_ = GeneratorOutputPath(output_name_);
    SPDLOG_INFO("taskid:{} data:{} rank:{} members_size:{}  ips_size:{} fields_size:{} labels_size:{} parties_:{} output_name_:{} output_path_:{} callbackUrl:{}"
               ,parma_.task_id,parma_.data,parma_.rank
               ,parma_.members.size(),parma_.ips.size(),parma_.fields.size(),parma_.labels.size()
               ,parties_,output_name_,output_path_,parma_.callbackUrl);
    response_->set_task_id(parma_.task_id);
    response_->set_status(200);
    return true;
}

void PirClient::ResultCallback()
{
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
    callback.set_spend(time_spend_);
    callback.mutable_data_result()->set_dataset_name(output_name_);
    std::string body;
    SPDLOG_INFO("callback body:{} taskid:{}",callback.DebugString(),parma_.task_id);
    //cntl.request_attachment().append(body);
    channel.CallMethod(NULL, &cntl, &callback, NULL, NULL/*done*/);
    if (!cntl.Failed()) 
    {
        SPDLOG_INFO( "taskid:{} Received response from {} to {} : (attached={}) latency={}  us" ,
        parma_.task_id,cntl.remote_side(), cntl.local_side(), 
        cntl.response_attachment(),cntl.latency_us());
    }
    else
    {
        SPDLOG_INFO("taskid:{} error{} ",parma_.task_id,cntl.ErrorText());
    }
}


}
}