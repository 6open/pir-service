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

#include "db_client.h"
void DbClient::Run()
{
    // time_profiler_.Disable();
    std::string status = recv_func_("meta_info");
    if(status != "ok"){
        SPDLOG_INFO("recv meta_info:{} error task_id:{}",status,task_id_);
        return;
    }
    LoadClientData();
    if(GetBatchInfo() < 0)
    {
        return ;
    }
    DispatchBatch(query_item_);
    
    RecvTimeProfile();
}

void DbClient::LoadClientData()
{
    

    std::vector<std::string> query_data;
    std::istringstream ss(query_ids_);
    std::string token;

    while (std::getline(ss, token, ',')) {
        query_item_.push_back(token);
    }
    SPDLOG_INFO("query_ids_ {}", query_item_.size());

    return ;
}

int DbClient::GetBatchInfo()
{
    std::string str_batch_info = recv_func_("batch_info");
    batch_helper_.DeserializeBatchHash(str_batch_info);
    if(batch_helper_.total_batch_size_ == 0){
        
        SPDLOG_ERROR("The batch size is zero, return directly");
        send_func_("STOP RPC", "STOP RPC");
        return -1;
    }
    result_writer_.EnableClientFilter(batch_helper_.label_columns_);
    part_result_writer_.EnableClientFilter(batch_helper_.label_columns_);
    SPDLOG_INFO("recv batch_info:total:{} real:{} task_id:{}",batch_helper_.total_batch_size_,batch_helper_.merged_batch_size_,task_id_);

    
    return 0;
}

void DbClient::DispatchBatch(const std::vector<std::string>& query_item)
{
    size_t query_batch_count = GetTotalQueryBatch(query_item_.size());
    int repeat_count = 1;
    if(query_batch_count%batch_helper_.total_batch_size_)
        repeat_count = query_batch_count/batch_helper_.total_batch_size_ + 1;
    else
        repeat_count = query_batch_count/batch_helper_.total_batch_size_;
    send_func_("repeat_count",std::to_string(repeat_count));

    batch_item_.resize(batch_helper_.merged_batch_size_ * repeat_count);
    int i = 0;
    for(const std::string& str : query_item)
    {
        std::size_t batch_count = GetBatchCount(str,batch_helper_.total_batch_size_);
        std::size_t real_count = batch_helper_.GetMergedBatch(batch_count) + i;
        batch_item_[real_count].push_back(str);
        if(++i % repeat_count == 0) i = 0;
        // SPDLOG_INFO("real_count : {}", real_count);
    }

    // result_.resize(batch_size_);
    SPDLOG_INFO("batch_item_.size() : {}", batch_item_.size());
    for(std::size_t i=0;i<batch_item_.size();++i)
    {
        QueryBatch(i/repeat_count,batch_item_[i]);
    }
    result_writer_.Release();
    part_result_writer_.Release();
}

void DbClient::QueryBatch(std::size_t batch_count,const std::vector<std::string>& query_item)
{
    // time_profiler_.Count("send_query_batch",ProType::NETWORK);
    send_func_(GeneratorKey(batch_count,"query_batch"),std::to_string(query_item.size()));
    if(query_item.empty())
        return;
    time_profiler_.Count("generator_oprf_request",ProType::ALGO);

    std::string parma(kPirFseParma);
    apsi::wrapper::APSIClientWrapper client_wrapper(parma);
    std::string oprf_request = client_wrapper.oprf_request(query_item);
    SPDLOG_INFO("send oprf_request size:{} task_id:{} batch_count:{}",oprf_request.size(),task_id_,batch_count);
    time_profiler_.Count("send_oprf_request",ProType::NETWORK);
    send_func_(GeneratorKey(batch_count,"oprf_request"),oprf_request);

    time_profiler_.Count("recv_oprf_response",ProType::NETWORK);
    std::string oprf_response = recv_func_(GeneratorKey(batch_count,"oprf_response"));
    SPDLOG_INFO("recv oprf_response size:{} task_id:{} batch_count:{}",oprf_response.size(),task_id_,batch_count);

    time_profiler_.Count("generator_query_string",ProType::ALGO);
    std::string query_string = client_wrapper.build_query(oprf_response);
    SPDLOG_INFO("send query_string size:{} task_id:{} batch_count:{}",query_string.size(),task_id_,batch_count);
    time_profiler_.Count("send_query_string",ProType::NETWORK);
    send_func_(GeneratorKey(batch_count,"query_string"),query_string);
    time_profiler_.Count("recv_response_string",ProType::NETWORK);
    std::string response_string = recv_func_(GeneratorKey(batch_count,"response_string"));

    SPDLOG_INFO("recv response_string size:{} task_id:{} batch_count:{}",response_string.size(),task_id_,batch_count);
    // result_[batch_count] = client_wrapper.extract_labeled_result(response_string);
    time_profiler_.Count("save_result",ProType::LOAD_CSV);
    SaveResult(query_item,client_wrapper.extract_labeled_result(response_string));
    time_profiler_.Flush();
}


void rtrim(std::string &s){
    size_t pos = s.find_first_of('\0');
    s = s.substr(0, pos);
}

void DbClient::SaveResult(const std::vector<std::string>& fileds,const std::vector<std::string>& labels)
{
    SPDLOG_INFO("labels size : {}, part_result_num : {}", labels.size(), part_result_num);
    for(std::size_t i=0;i<labels.size();++i)
    {
        if(!labels[i].empty())
        {
            std::string tmp_label = labels[i];
            rtrim(tmp_label);
            result_writer_.AddItem(fileds[i],tmp_label);
            if(i < part_result_num)
            {
                part_result_writer_.AddItem(fileds[i],tmp_label);
            }
        }
    }
}

void DbClient::RecvTimeProfile()
{
    server_profile = recv_func_("time_profile");
}

std::size_t DbClient::GetBatchCount(const std::string& str,std::size_t batch_size)
{
    // SPDLOG_INFO("str {}, batch_size {}", str, batch_size);
    // return (*reinterpret_cast<const uint64_t*>(str.c_str())) % batch_size;
   
    std::hash<std::string> hasher;
    uint64_t hash_value = hasher(str);
    return hash_value % batch_size;
}

std::size_t DbClient::GetTotalQueryBatch(size_t query_item_size)
{
    //apsi每次查询的数据量 * 1.2 不得大于参数中的table_size
    std::vector<std::vector<std::string>> query_part;
    nlohmann::json json_data = nlohmann::json::parse(kPirFseParma);
    int table_size = json_data["table_params"]["table_size"];
    size_t query_batch_size = table_size * 10 / 12;
    size_t query_batch_count = query_item_size/query_batch_size;
    if(query_item_size%query_batch_size) query_batch_count++;
    return query_batch_count;
}

