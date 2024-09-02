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
#include <fmt/format.h>



class MegaBatch
{
public:
    MegaBatch(const std::string& bt_name, std::size_t bt_size, std::size_t bt_count);
    MegaBatch() = default;
    std::size_t TotalSize() const;
    void Serialize(std::ofstream& out) const;
    static MegaBatch Deserialize(std::ifstream& in);
    bool operator<(const MegaBatch& other) const;
    bool operator>(const MegaBatch& other) const;
    MegaBatch& operator+=(const MegaBatch& other);
    MegaBatch(const MegaBatch& other);
    MegaBatch& operator=(const MegaBatch& other);
    MegaBatch(MegaBatch&& other) noexcept;
    MegaBatch& operator=(MegaBatch&& other) noexcept;
    bool operator==(const MegaBatch& rhs) const;
    bool operator!=(const MegaBatch& other) const;
    void SetSelfNum(std::size_t self_num) {self_num_ = self_num;}
    void SetDataCacheName(const std::string& name) {data_cache_name_ = name;}
    std::string DataCacheName() const {return data_cache_name_;}
public:
    std::size_t total_size=0;
    std::size_t self_num_=0;
    std::string data_cache_name_;
    std::vector<std::string> batch_names_;
    std::vector<std::size_t> per_batch_size_;
    std::vector<std::size_t> batch_count_;
friend class DbMetaInfoTest;
};
template <>
struct fmt::formatter<MegaBatch> {
    // 格式化函数
    template <typename FormatContext>
    auto format(const MegaBatch& obj, FormatContext& ctx) {
        // 将 MyClass 对象的信息格式化为字符串
        return format_to(ctx.out(), "MegaBatch(data_cache_name_={},total_size={},self_num_:{},batch_names_:{},per_batch_size_:{},batch_count_:{})"
        , obj.data_cache_name_,obj.total_size,obj.self_num_
        ,fmt::join(obj.batch_names_,","),fmt::join(obj.per_batch_size_,","),fmt::join(obj.batch_count_,","));
    }
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.begin();
  }
};