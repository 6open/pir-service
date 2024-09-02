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
#pragma once
#include<functional>
#include<string>
#include<vector>
#include <sstream>
#include <nlohmann/json.hpp>
#include "batch_writer.h"
#include "yacl/io/rw/csv_reader.h"
#include "yacl/io/rw/csv_writer.h"
#include "yacl/io/stream/file_io.h"
#include "apsi_wrapper.h"
#include "src/pir/pir_fse_prama.h"
#include "spdlog/spdlog.h"
#include "time_profiler.h"
#include "db_meta_info.h"


class DbClient
{
public:
using SendFunc = std::function<void(const std::string& key,const std::string& message)>;
using RecvFunc = std::function<std::string(const std::string& key)>;
    DbClient(const std::string& data_path,const std::string& query_ids,const std::string& result_path,const std::string& part_result_path,
    const std::string& task_id,const std::vector<std::string>& fields,const std::vector<std::string>& labels,
    const SendFunc& send_func,const RecvFunc& recv_func)
    :data_path_(data_path)
    ,query_ids_(query_ids)
    ,result_path_(result_path)
    ,part_result_path_(part_result_path)
    ,task_id_(task_id)
    ,fields_(fields)
    ,labels_(labels)
    ,send_func_(send_func)
    ,recv_func_(recv_func)
    ,result_writer_(result_path_,fields_,labels_)
    ,part_result_writer_(part_result_path_,fields_,labels_)
    {

    }

    void Run();
    int GetBatchInfo();
    void LoadClientData();
    void DispatchBatch(const std::vector<std::string>& query_item);
    void Query();
    void QueryBatch(std::size_t batch_count,const std::vector<std::string>& query_item);
    std::string GeneratorKey(std::size_t num,const std::string& str)
    {
        return std::to_string(num)+"_"+str;
    }
    std::size_t GetBatchCount(const std::string& str,std::size_t batch_size);
    std::size_t GetTotalQueryBatch(size_t query_item_size);
    void SaveResult(const std::vector<std::string>& fileds,const std::vector<std::string>& labels);
    void RecvTimeProfile();
    std::string ServerProfile() {return server_profile;}
    std::string ClientProfile() {return time_profiler_.ProtoString();}
private:
    std::string data_path_;
    std::string query_ids_;
    std::string result_path_;
    std::string part_result_path_;
    std::string task_id_;
    std::vector<std::string> fields_;
    std::vector<std::string> labels_;
    SendFunc send_func_;
    RecvFunc recv_func_;
    BatchWriter result_writer_;
    BatchWriter part_result_writer_;

    // ColumnVectorBatch batch;
    std::vector<std::string> query_item_;
    // std::size_t batch_size_;
    std::vector<std::vector<std::string>> batch_item_;
    // std::vector<std::vector<std::string>> result_;
    DbBatchHashHelper batch_helper_;

    TimeProfiler time_profiler_;
    std::string server_profile;
};