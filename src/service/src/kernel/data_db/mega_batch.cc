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

#include "mega_batch.h"

#include <fstream>
#include <iostream>
#include <type_traits>

MegaBatch::MegaBatch(const std::string& bt_name, std::size_t bt_size, std::size_t bt_count)
    : total_size(bt_size)
    , self_num_(0)
{
    batch_names_.emplace_back(bt_name);
    per_batch_size_.emplace_back(bt_size);
    batch_count_.emplace_back(bt_count);
}

std::size_t MegaBatch::TotalSize() const
{
    return total_size;
}

void MegaBatch::Serialize(std::ofstream& out) const
{
    out.write(reinterpret_cast<const char*>(&total_size), sizeof(std::size_t));

        // 写入self_num_
    out.write(reinterpret_cast<const char*>(&self_num_), sizeof(std::size_t));

    // 写入batch_names_的大小和内容
    std::size_t size = data_cache_name_.size();
    out.write(reinterpret_cast<const char*>(&size), sizeof(size));
    out.write(data_cache_name_.c_str(), size);
    std::size_t batchNamesSize = batch_names_.size();
    out.write(reinterpret_cast<const char*>(&batchNamesSize), sizeof(std::size_t));
    for (const auto& name : batch_names_)
    {
        std::size_t nameSize = name.size();
        out.write(reinterpret_cast<const char*>(&nameSize), sizeof(std::size_t));
        out.write(name.c_str(), nameSize);
    }

    // 写入per_batch_size_的大小和内容
    std::size_t perBatchSizeSize = per_batch_size_.size();
    out.write(reinterpret_cast<const char*>(&perBatchSizeSize), sizeof(std::size_t));
    out.write(reinterpret_cast<const char*>(per_batch_size_.data()), per_batch_size_.size() * sizeof(std::size_t));

    // 写入batch_count_的大小和内容
    std::size_t batchCountSize = batch_count_.size();
    out.write(reinterpret_cast<const char*>(&batchCountSize), sizeof(std::size_t));
    out.write(reinterpret_cast<const char*>(batch_count_.data()), batch_count_.size() * sizeof(std::size_t));
    // out<<std::endl;
}

MegaBatch MegaBatch::Deserialize(std::ifstream & in)
{
    MegaBatch megabatch;
    in.read(reinterpret_cast<char*>(&megabatch.total_size), sizeof(std::size_t));

        // 读取self_num_
    in.read(reinterpret_cast<char*>(&megabatch.self_num_), sizeof(std::size_t));

    std::size_t size;
    in.read(reinterpret_cast<char*>(&size), sizeof(size));
    megabatch.data_cache_name_.resize(size, '\0');
    in.read(&(megabatch.data_cache_name_[0]), size);

    // 读取batch_names_的大小和内容
    std::size_t batchNamesSize;
    in.read(reinterpret_cast<char*>(&batchNamesSize), sizeof(std::size_t));
    for (std::size_t i = 0; i < batchNamesSize; ++i)
    {
        std::size_t nameSize;
        in.read(reinterpret_cast<char*>(&nameSize), sizeof(std::size_t));
        std::string name(nameSize, '\0');
        in.read(&name[0], nameSize);
        megabatch.batch_names_.push_back(name);
    }

    // 读取per_batch_size_的大小和内容
    std::size_t perBatchSizeSize;
    in.read(reinterpret_cast<char*>(&perBatchSizeSize), sizeof(std::size_t));
    megabatch.per_batch_size_.resize(perBatchSizeSize);
    in.read(reinterpret_cast<char*>(megabatch.per_batch_size_.data()), perBatchSizeSize * sizeof(std::size_t));

    // 读取batch_count_的大小和内容
    std::size_t batchCountSize;
    in.read(reinterpret_cast<char*>(&batchCountSize), sizeof(std::size_t));
    megabatch.batch_count_.resize(batchCountSize);
    in.read(reinterpret_cast<char*>(megabatch.batch_count_.data()), batchCountSize * sizeof(std::size_t));

    // std::string line;
    // std::getline(in,line);

    return megabatch;
}

bool MegaBatch::operator<(const MegaBatch& other) const
{
    return total_size < other.total_size;
}

bool MegaBatch::operator>(const MegaBatch& other) const
{
    return total_size > other.total_size;
}

MegaBatch& MegaBatch::operator+=(const MegaBatch& other)
{
    if (this != &other)
    {
        total_size += other.total_size;
        batch_names_.insert(batch_names_.end(), other.batch_names_.begin(), other.batch_names_.end());
        per_batch_size_.insert(per_batch_size_.end(), other.per_batch_size_.begin(), other.per_batch_size_.end());
        batch_count_.insert(batch_count_.end(), other.batch_count_.begin(), other.batch_count_.end());
    }
    return *this;
}

MegaBatch::MegaBatch(const MegaBatch& other)
    : total_size(other.total_size),
      self_num_(other.self_num_),
      data_cache_name_(other.data_cache_name_),
      batch_names_(other.batch_names_),
      per_batch_size_(other.per_batch_size_),
      batch_count_(other.batch_count_)
{
}

MegaBatch& MegaBatch::operator=(const MegaBatch& other)
{
    if (this != &other)
    {
        total_size = other.total_size;
        self_num_ = other.self_num_;
        data_cache_name_ = other.data_cache_name_;
        batch_names_ = other.batch_names_;
        per_batch_size_ = other.per_batch_size_;
        batch_count_ = other.batch_count_;
    }
    return *this;
}

MegaBatch::MegaBatch(MegaBatch&& other) noexcept
    : total_size(other.total_size),
      self_num_(other.self_num_),
      data_cache_name_(std::move(other.data_cache_name_)),
      batch_names_(std::move(other.batch_names_)),
      per_batch_size_(std::move(other.per_batch_size_)),
      batch_count_(std::move(other.batch_count_))
{
    other.total_size = 0;
}

MegaBatch& MegaBatch::operator=(MegaBatch&& other) noexcept
{
    if (this != &other)
    {
        total_size = other.total_size;
        self_num_ = other.self_num_;
        data_cache_name_= (std::move(other.data_cache_name_));
        batch_names_ = std::move(other.batch_names_);
        per_batch_size_ = std::move(other.per_batch_size_);
        batch_count_ = std::move(other.batch_count_);

        other.total_size = 0;
    }
    return *this;
}

bool MegaBatch::operator==(const MegaBatch& rhs) const
{
    if(this == &rhs)
        return true;
    if(this->data_cache_name_ != rhs.data_cache_name_)
    {
        return false;
    }
    if(this->total_size != rhs.total_size || this->self_num_ != rhs.self_num_)
        return false;
    if(!(this->batch_names_.size() == rhs.batch_names_.size() && std::equal(this->batch_names_.begin(), this->batch_names_.end(), rhs.batch_names_.begin()) ) )
        return false;
    if(!(this->per_batch_size_.size() == rhs.per_batch_size_.size() && std::equal(this->per_batch_size_.begin(), this->per_batch_size_.end(), rhs.per_batch_size_.begin()) ) )
        return false;
    if(!(this->batch_count_.size() == rhs.batch_count_.size() && std::equal(this->batch_count_.begin(), this->batch_count_.end(), rhs.batch_count_.begin()) ) )
        return false;
    return true;
}

bool MegaBatch::operator!=(const MegaBatch& rhs) const
{
    return !(*this == rhs);
}

