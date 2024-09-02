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
#include "json.h"
#include <fstream>

json JsonFromFile(const std::string& file_path)
{
    std::ifstream f(file_path);
    return json::parse(f);
}

void JsonToFile(const json& data,const std::string& file_path)
{
    std::ofstream out(file_path);
    out<<data.dump(4);
}

std::string JsonGetString(const json& data,const std::string& key,std::string default_value) noexcept
{
    if(data.find(key) != data.end() && data[key].is_string())
        return data[key];
    else
        return default_value;
}

int JsonGetInt(const json& data,const std::string& key,int default_value) noexcept
{
    if(data.find(key) != data.end() && data[key].is_number())
        return data[key];
    else
        return default_value;
}

int JsonGetIntNoCast(const json& data,const std::string& key,int default_value) noexcept
{
    if(data.find(key) != data.end() && data[key].is_number_integer())
        return data[key];
    else
        return default_value;   
}