// Copyright 2021 Ant Group Co., Ltd.
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

#include "db_server.h"


void DbServer::Run()
{
    meta_ = DbMetaManager::Instance().GetDbMeta(key_);
    if(meta_ == nullptr)
    {
        SPDLOG_INFO(" can not get meta task_id:{}",task_id_);
        send_func_("meta_info","no");
        return;
    }
    else
        send_func_("meta_info","ok");

    send_func_("batch_info",DbBatchHashHelper::SerializeBatchHash(meta_->MetaInfo()));
    std::string repeat_count = recv_func_("repeat_count");
    HandleBatchQuery(meta_->MetaInfo().MergedBatchSize(), std::stoi(repeat_count));
    SendTimeProfile();
}

void DbServer::HandleBatchQuery(std::size_t batch_size, std::size_t repeat_count)
{
    for(std::size_t batch_count=0;batch_count<batch_size;++batch_count)
    {
        for (std::size_t j = 0; j < repeat_count; j++) {
            time_profile_.Count("recv_query_batch",ProType::NETWORK);
            std::string query_num_str = recv_func_(GeneratorKey(batch_count,"query_batch"));
            std::size_t query_num = std::stoull(query_num_str);
            if(query_num == 0)
                continue;
            // std::size_t real_batch = meta_->MetaInfo().GetMergedBatch(i);
            // time_profile_.Count("load_db",ProType::LOAD_DB);
            DataCacher* data_cacher = meta_->GetDataCacher(batch_count);
            if(data_cacher->Loading() == false){
                SPDLOG_ERROR("Load cacher faield");
                return ;
            }
            SeDataCacher* se_cacher = dynamic_cast<SeDataCacher*>(data_cacher);
            apsi::wrapper::APSIServerWrapper& server_wrapper = *(se_cacher->GetApsiServer());
            time_profile_.Count("recv_oprf_request",ProType::NETWORK);
            std::string oprf_request = recv_func_(GeneratorKey(batch_count,"oprf_request"));
            SPDLOG_INFO(" recv oprf_request size:{} task_id:{} batch_count:{}",oprf_request.size(),task_id_,batch_count);
            time_profile_.Count("generator_oprf_response",ProType::ALGO);
            std::string oprf_response = server_wrapper.handle_oprf_request(oprf_request);
            SPDLOG_INFO(" send oprf_response size:{} task_id:{} batch_count:{}",oprf_response.size(),task_id_,batch_count);
            time_profile_.Count("sebd_oprf_response",ProType::NETWORK);
            send_func_(GeneratorKey(batch_count,"oprf_response"),oprf_response);
            time_profile_.Count("recv_query_string",ProType::NETWORK);
            std::string query_string = recv_func_(GeneratorKey(batch_count,"query_string"));
            SPDLOG_INFO(" recv query_string size:{} task_id:{} batch_count:{}",query_string.size(),task_id_,batch_count);
            time_profile_.Count("generator_response_string",ProType::ALGO);
            std::string response_string = server_wrapper.handle_query(query_string);
            SPDLOG_INFO(" send response_string size:{} task_id:{} batch_count:{}",response_string.size(),task_id_,batch_count);
            time_profile_.Count("send_response_string",ProType::NETWORK);
            send_func_(GeneratorKey(batch_count,"response_string"),response_string);
            time_profile_.Flush();
            data_cacher->Release();
        }
    }
}   


void DbServer::SendTimeProfile()
{
    send_func_("time_profile",time_profile_.ProtoString());
}
