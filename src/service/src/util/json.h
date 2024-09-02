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

#include <nlohmann/json.hpp>

using json = nlohmann::json;


json JsonFromFile(const std::string& file_path);
void JsonToFile(const json& data,const std::string& file_path);

std::string JsonGetString(const json& data,const std::string& key,std::string default_value="") noexcept;
int JsonGetInt(const json& data,const std::string& key,int default_value=0) noexcept;
int JsonGetIntNoCast(const json& data,const std::string& key,int default_value=0) noexcept;

