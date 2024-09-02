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
#include <iostream>
#include<string>
#include<unordered_map>
#include<map>
#include<vector>

#include "yacl/io/rw/csv_reader.h"
#include "yacl/io/stream/file_io.h"
#include "spdlog/spdlog.h"

using namespace yacl::io;   
class DataOwner
{
public:
    DataOwner() =default;
    DataOwner(const std::string& name);
    bool init(const std::string& filename,const std::string& col_name,std::vector<std::string>& feature,std::string& errorMessage);
    std::vector<std::string> exit(const std::string& key);
    std::vector<std::vector<std::string>> exit(const std::vector<std::string>& keys,std::vector<std::string>& feature);
    std::string md5(const std::string& str);

private:
    std::string name_;
    std::unordered_map<std::string,int> data_;
    std::vector<std::string> feature_;
    ColumnVectorBatch batch_out_;
    int feature_size = 0;
};
class SearchManager
{
public:
    bool Add(const std::string& name,const std::string& filename,const std::string& col_name,std::vector<std::string>& feature,std::string& errorMessage)
    {
        maps[name] = DataOwner(name);
        return maps[name].init(filename,col_name,feature,errorMessage);
    }
    void Del(const std::string& name)
    {
        maps.erase(name);
    }
    std::vector<std::vector<std::string>> exit(const std::string& name,const std::vector<std::string>& keys,std::vector<std::string>& feature,std::string& errorMessage)
    {
        auto iter = maps.find(name);
        errorMessage.clear();
        if(iter != maps.end())
        {
            return iter->second.exit(keys,feature);
        }
        else
        {
            errorMessage = name + std::string(" not exited ");
            SPDLOG_INFO("search {} not exited",name);
            return std::vector<std::vector<std::string>>();
        }
    }
private:
    std::map<std::string,DataOwner> maps;
};