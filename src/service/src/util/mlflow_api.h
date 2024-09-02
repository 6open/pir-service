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
#include <string>
#include <vector>

std::vector<std::string> get_experiment_filenames(const std::string& mlflow_addr, const std::string& exp_name);
bool downloadMlflowFiles(const std::string& mlflow_addr, const std::string& exp_name, const std::string& out_path);
bool downloadMlflowFileByName(const std::string& mlflow_addr, const std::string& exp_name, 
                              const std::string& target_file_name, const std::string& out_path, const std::string& out_name);
bool uploadMlflowFiles(const std::string& mlflow_addr, const std::string& folder, std::string exp_name="");
std::vector<std::string> vecFromFile(const std::string& filePath);
void vecToFile(const std::vector<std::string>& data, const std::string& filePath);
void strToFile(const std::string& data, const std::string& filePath) ;