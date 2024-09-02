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
#include <queue>
#include <fstream>
#include "mega_batch.h"
const std::size_t kPerBatchMinSize = 1000000;
const std::size_t part_result_num = 20;

class DbMetaInfo
{
public:
    DbMetaInfo(const std::string& key,const std::string& meta_path,
    const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns);
    void SetBatch( const std::vector<std::string>& batch_names,const std::vector<std::size_t>& num_count);

    DbMetaInfo() = default;

    bool operator==(const DbMetaInfo& other) const;

    void MergeBatch(const std::vector<std::string>& batch_names,const std::vector<std::size_t>& num_count);
    void InitHashTable();

    void Serialize(const std::string& filename);
    void Deserialize(const std::string& filename);
    // static  Load();
    static std::string CreateDbMetaInfoPath(const std::string& meta_path);
    static std::string CreateBatchName(const std::string& key,const std::string& meta_path,std::size_t num);

    std::size_t BatchSize() const {return total_batch_size_;}
    std::size_t MergedBatchSize() const {return merged_batch_size_;}
    std::size_t GetMergedBatch(std::size_t expect_batch) const {return batch_hash_[expect_batch];}
public:
    std::string key_;
    std::string meta_path_;
    std::vector<MegaBatch> mega_batchs_;
    std::size_t total_batch_size_;
    std::size_t merged_batch_size_;
    std::vector<std::size_t> batch_hash_;
    std::vector<std::string> key_columns_;
    std::vector<std::string> label_columns_;
friend class DbMetaInfoTest;
};

class DbBatchHashHelper
{
public:
    static std::string SerializeBatchHash(const DbMetaInfo& info);
    void DeserializeBatchHash(const std::string& info);
    std::size_t GetMergedBatch(std::size_t expect_batch) const {return batch_hash_[expect_batch];}
public:
    std::vector<std::size_t> batch_hash_;
    std::vector<std::string> label_columns_;
    std::size_t total_batch_size_;
    std::size_t merged_batch_size_;
    std::size_t repeat_count_;
};