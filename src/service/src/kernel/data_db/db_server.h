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
#include "apsi_wrapper.h"
#include "spdlog/spdlog.h"
#include "db_meta.h"
#include "time_profiler.h"


class DbServer
{
public:
using SendFunc = std::function<void(const std::string& key,const std::string& message)>;
using RecvFunc = std::function<std::string(const std::string& key)>;
    DbServer(const std::string& key,const std::string& task_id,
        const SendFunc& send_func,const RecvFunc& recv_func)
    :key_(key)
    ,task_id_(task_id)
    ,send_func_(send_func)
    ,recv_func_(recv_func)
    {}
    void Run();
    void SyncBatchInfo(const DbMetaInfo& info);
    void HandleBatchQuery(std::size_t batch_size, std::size_t repeat_count);
    std::string GeneratorKey(std::size_t num,const std::string& str)
    {
        return std::to_string(num)+"_"+str;
    }
    std::size_t GetBatchCount(const std::string& str,std::size_t batch_size)
    {
        // return  (*reinterpret_cast<const uint64_t*>(str.c_str())) % batch_size;
        std::hash<std::string> hasher;
        uint64_t hash_value = hasher(str);
        return hash_value % batch_size;
    }
    void SendTimeProfile();
private:
    std::string key_;
    std::string task_id_;
    SendFunc send_func_;
    RecvFunc recv_func_;
    DbMeta* meta_ = nullptr;
    TimeProfiler time_profile_;
};