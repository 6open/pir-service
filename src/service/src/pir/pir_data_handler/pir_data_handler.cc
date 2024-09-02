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

#include "pir_data_handler.h"

namespace mpc{
namespace pir{

bool PirDataHandlerBase::CheckParma(const PirSetupRequest* request, PirSetupResponse* response)
{
    parma_.version = request->version();
    parma_.task_id = request->task_id();
    parma_.data_file = request->data_file();
    parma_.algorithm = request->algorithm();
    parma_.callbackUrl = request->callback_url();
    for(int i = 0;i<request->fields_size();++i)
        parma_.fields.push_back(request->fields(i));
    for(int i = 0;i<request->labels_size();++i)
        parma_.labels.push_back(request->labels(i));
    // for(std::size_t i=0;i<parma_.labels.size();++i)
    //     SPDLOG_INFO("get labels:{} ",parma_.labels[i]);
    //parma_.ips[parma_.rank] = "0.0.0.0";
    SPDLOG_INFO("taskid:{} data:{} key:{} fields_size:{} labels_size:{} callbackUrl:{}"
            ,parma_.task_id,parma_.data_file,GetKey()
            ,parma_.fields.size(),parma_.labels.size()
            ,parma_.callbackUrl);
    if(parma_.fields.empty())
    {
        response->set_status(201);
        response->set_message("fields empty");
        return false;
    }
    if(parma_.callbackUrl.empty())
    {
        response->set_status(201);
        response->set_message("callback url empty");
        return false;
    }
    brpc::ChannelOptions options;
    options.timeout_ms = 1000;
    options.protocol = brpc::PROTOCOL_HTTP;
    if (callback_channel_.Init(parma_.callbackUrl.c_str(), &options) != 0)
    {
        response->set_status(201);
        response->set_message("callbackurl invailded: "+parma_.callbackUrl);
        return false;
    }
    response->set_status(200);
    return true;
}

void PirDataHandlerBase::Setup() noexcept
{
    auto start_time = std::chrono::high_resolution_clock::now();
    SetupImpl();
    auto end_time = std::chrono::high_resolution_clock::now();
    auto time_diff = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    time_spend_ = time_diff.count();
    SPDLOG_INFO(" time cost:{} milliseconds,taskid:{}",time_spend_,parma_.task_id);
    ResultCallback();
}

void PirDataHandlerBase::ResultCallback()
{
    SPDLOG_INFO("begin callback status_:{} taskid:{}",status_,parma_.task_id);
    brpc::Controller cntl;
    PirSetupCallbackRequest callback;
    PirSetupCallbackResponse response;
    cntl.http_request().uri() = parma_.callbackUrl;  // 设置为待访问的URL
    cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
    callback.set_status(status_);
    callback.set_message(error_message_);
    callback.set_task_id(parma_.task_id);
    callback.set_service_type("PIR");
    callback.set_spend(time_spend_);
    callback.mutable_data_result()->set_algorithm(parma_.algorithm);
    callback.mutable_data_result()->set_key(GetKey());
    std::string body;
    SPDLOG_INFO("callback status_:{} body:{} taskid:{}",status_,callback.DebugString(),parma_.task_id);
    //cntl.requestattachment().append(body);
    callback_channel_.CallMethod(NULL, &cntl, &callback, &response, NULL/*done*/);
    if (!cntl.Failed() && response.code() == 0) 
    {
        SPDLOG_INFO( "taskid:{} Received response" ,parma_.task_id);
    }
    else
    {
        SPDLOG_ERROR("setup callback failed taskid:{} error{} code:{} message:{}",parma_.task_id,cntl.ErrorText(),response.code(),response.message());
        // remove_callback_(GetKey());
    }
}

}
}