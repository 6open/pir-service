// Copyright 2022 Ant Group Co., Ltd.
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
#include <fstream>
#include "absl/strings/string_view.h"
#include "absl/strings/str_split.h"
const std::size_t kMaxItemSize = 4096;
class BatchWriter
{
public:
    BatchWriter(const std::string& file_path,const std::vector<std::string>& fileds,const std::vector<std::string>& labels,
                char field_delimiter = ',', char line_delimiter = '\n');
    ~BatchWriter();
    void AddItem(const std::string& key_values,const std::string& label_values);
    void Release();
    std::size_t TotalCount() const {return total_count_;}

    void EnableClientFilter(const std::vector<std::string>& server_labels);
private:
    void WriteHeader();
    void Flush();

private:
    const std::string& file_path_;
    std::ofstream out_;
    const std::vector<std::string>& fileds_;
    const std::vector<std::string>& labels_;
    const std::string field_delimiter_;
    const std::string line_delimiter_;
    std::vector<std::string> item_key_;
    std::vector<std::string> item_label_;
    std::size_t vec_count_=0;
    std::size_t total_count_ = 0;
    bool enable_client_filter_ = false;
    std::vector<std::size_t> label_index_;
    std::vector<std::vector<absl::string_view>> split_item_label_;

};