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

#include "db_meta_info.h"

#include <fstream>
#include <iostream>
#include <type_traits>
#include "spdlog/spdlog.h"
#include "src/kernel/data_db/time_profiler.pb.h"


// 辅助函数：序列化 std::string
void SerializeString(std::ofstream& file, const std::string& str)
{
    std::size_t size = str.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.write(str.c_str(), size);
}

// 辅助函数：反序列化 std::string
void DeserializeString(std::ifstream& file, std::string& str)
{
    std::size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    str.resize(size);
    file.read(&str[0], size);
}

void SerializeStringVector(std::ofstream& file, const std::vector<std::string>& vec)
{
    std::size_t size = vec.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& str : vec)
    {
        std::size_t strSize = str.size();
        file.write(reinterpret_cast<const char*>(&strSize), sizeof(strSize));
        file.write(str.c_str(), strSize);
    }
}

void DeserializeStringVector(std::ifstream& file,std::vector<std::string>& vec)
{
    std::size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    vec.reserve(size);
    for (std::size_t i = 0; i < size; ++i)
    {
        std::size_t strSize;
        file.read(reinterpret_cast<char*>(&strSize), sizeof(strSize));
        std::string str(strSize, '\0');
        file.read(&str[0], strSize);
        vec.push_back(std::move(str));
    }
}

// 辅助函数：序列化 std::vector
template <typename T,typename std::enable_if_t<std::is_trivially_copyable_v<T>, int> N = 0>
// typename std::enable_if<std::is_trivially_copyable<T>::value>::type
void SerializeVector(std::ofstream& file, const std::vector<T>& vec)
{
    std::size_t size = vec.size();
    file.write(reinterpret_cast<const char*>(&size), sizeof(size));
    file.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
}

// 辅助函数：反序列化 std::vector
template <typename T,typename std::enable_if_t<std::is_trivially_copyable_v<T>, int> N = 0>
void DeserializeVector(std::ifstream& file, std::vector<T>& vec)
{
    std::size_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(size));
    vec.resize(size);
    file.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
}

DbMetaInfo::DbMetaInfo(const std::string& key,const std::string& meta_path,
    const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns)
:key_(key)
,meta_path_(meta_path)
,key_columns_(key_columns)
,label_columns_(label_columns)
{

}

void DbMetaInfo::SetBatch( const std::vector<std::string>& batch_names,const std::vector<std::size_t>& num_count)
{
    total_batch_size_ = batch_names.size();
    MergeBatch(batch_names, num_count);
    InitHashTable();
}

bool DbMetaInfo::operator==(const DbMetaInfo& other) const
{
    // 比较所有成员变量是否相等
    if(this == &other)
        return true;
    if(key_ != other.key_ 
    || meta_path_ != other.meta_path_  
    || total_batch_size_ != other.total_batch_size_
    || merged_batch_size_ != other.merged_batch_size_
    || mega_batchs_.size() != other.mega_batchs_.size()
    || batch_hash_.size() != other.batch_hash_.size()
    || key_columns_.size() != other.key_columns_.size()
    || label_columns_.size() != other.label_columns_.size())
        return false;
    for(std::size_t i=0;i<mega_batchs_.size();++i)
        if(mega_batchs_[i] != other.mega_batchs_[i])
            return false;
    for(std::size_t i=0;i<batch_hash_.size();++i)
        if(batch_hash_[i] != other.batch_hash_[i])
            return false;
    for(std::size_t i=0;i<key_columns_.size();++i)
        if(key_columns_[i] != other.key_columns_[i])
            return false;
    for(std::size_t i=0;i<label_columns_.size();++i)
        if(label_columns_[i] != other.label_columns_[i])
            return false;
    return true;
}

void DbMetaInfo::MergeBatch(const std::vector<std::string>& batch_names,const std::vector<std::size_t>& num_count)
{
    std::priority_queue<MegaBatch, std::vector<MegaBatch>, std::greater<MegaBatch>> queues;
    for (std::size_t i = 0; i < num_count.size(); ++i)
    {
        SPDLOG_INFO("DbMetaInfo::before i:{} batch_names:{} num_count:{}", i,batch_names[i],num_count[i]);
        queues.push(MegaBatch(batch_names[i], num_count[i], i));
    }
    while (queues.size() > 1 && queues.top().TotalSize() < kPerBatchMinSize)
    {
        MegaBatch tp_bt1 = queues.top();
        queues.pop();
        MegaBatch tp_bt2 = queues.top();
        queues.pop();
        // SPDLOG_INFO("DbMetaInfo::MergeBatch  bw1:{} bw2:{}", tp_bt1,tp_bt2);
        tp_bt1 += tp_bt2;
        queues.push(tp_bt1);
    }
    std::size_t real_count=0;
    while (!queues.empty())
    {
        mega_batchs_.emplace_back(queues.top());
        mega_batchs_.back().SetSelfNum(real_count);
        mega_batchs_.back().SetDataCacheName(CreateBatchName(key_,meta_path_,real_count));
        ++real_count;
        queues.pop();
    }
    for(std::size_t i=0;i<mega_batchs_.size();++i)
    {
        SPDLOG_INFO("DbMetaInfo::MergeBatch i:{} bw:{}", i,mega_batchs_[i]);
    }
}

void DbMetaInfo::InitHashTable()
{
    batch_hash_.resize(total_batch_size_,0);
    merged_batch_size_ = mega_batchs_.size();
    for(const MegaBatch& mb : mega_batchs_)
    {
        for(std::size_t batch_count : mb.batch_count_)
        {
            batch_hash_[batch_count] = mb.self_num_;
        }
    }
}

// 序列化到文件
void DbMetaInfo::Serialize(const std::string& filename)
{
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for serialization: " << filename << std::endl;
        return;
    }

    // 写入数据成员到文件
    SerializeString(file, key_);
    SerializeString(file, meta_path_);
    file.write(reinterpret_cast<const char*>(&total_batch_size_), sizeof(total_batch_size_));
    file.write(reinterpret_cast<const char*>(&merged_batch_size_), sizeof(merged_batch_size_));
    SerializeStringVector(file,key_columns_);
    SerializeStringVector(file,label_columns_);
    SerializeVector(file,batch_hash_);

    // 写入每个 MegaBatch 对象的数据
    std::size_t mb_size = mega_batchs_.size();
    file.write(reinterpret_cast<const char*>(&mb_size), sizeof(mb_size));
    for (const auto& mega_batch : mega_batchs_)
        mega_batch.Serialize(file);
    file.close();
}

// 从文件反序列化
void DbMetaInfo::Deserialize(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open())
    {
        std::cerr << "Failed to open file for deserialization: " << filename << std::endl;
        return;
    }

    // 读取数据成员从文件
    DeserializeString(file, key_);
    DeserializeString(file, meta_path_);
    file.read(reinterpret_cast<char*>(&total_batch_size_), sizeof(total_batch_size_));
    file.read(reinterpret_cast<char*>(&merged_batch_size_), sizeof(merged_batch_size_));
    DeserializeStringVector(file,key_columns_);
    DeserializeStringVector(file,label_columns_);
    DeserializeVector(file,batch_hash_);
    std::size_t mb_size;
    file.read(reinterpret_cast<char*>(&mb_size), sizeof(mb_size));
    for(std::size_t i=0;i<mb_size;++i)
    {
        mega_batchs_.emplace_back(MegaBatch::Deserialize(file));
    }
    if(merged_batch_size_ == 0){
        SPDLOG_INFO("DbMetaInfo::Deserialize set merged_batch_size_:{} to mega_batchs_.size:{}", merged_batch_size_,mega_batchs_.size());
        merged_batch_size_ = mega_batchs_.size();
    }
    file.close();
}

std::string DbMetaInfo::CreateDbMetaInfoPath(const std::string& meta_path)
{
    return meta_path+"/metainfo";
    // return 
}

std::string DbMetaInfo::CreateBatchName(const std::string& key,const std::string& meta_path,std::size_t num)
{
    return meta_path+"/mega_btch."+std::to_string(num);
}

std::string DbBatchHashHelper::SerializeBatchHash(const DbMetaInfo& info)
{
    BatchHashMessage message;
    for (const auto& element : info.batch_hash_) {
        message.add_batch_hash(element);
    }
    for (const auto& element : info.label_columns_) {
        message.add_server_labels(element);
    }
    message.set_merged_batch_size(info.merged_batch_size_);
    message.set_total_batch_size(info.total_batch_size_);
    return message.SerializeAsString();
}

void DbBatchHashHelper::DeserializeBatchHash(const std::string& info)
{
    BatchHashMessage message;
    batch_hash_.clear();
    message.ParseFromString(info);
    for (int i = 0; i < message.batch_hash_size(); ++i) {
        batch_hash_.push_back(message.batch_hash(i));
    }
    for (int i = 0; i < message.server_labels_size(); ++i) {  
        label_columns_.push_back(message.server_labels(i));
    }
    total_batch_size_ = message.total_batch_size();
    merged_batch_size_ = message.merged_batch_size();
}