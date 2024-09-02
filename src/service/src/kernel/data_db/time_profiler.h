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

#include <chrono>
#include <map>
#include <string>
#include <vector>
#include <numeric>
#include <iostream>
#include <fmt/format.h>
#include "src/kernel/data_db/time_profiler.pb.h"
enum class ProType
{
    NETWORK = 0,
    LOAD_DB = 1,
    ALGO =2,
    LOAD_CSV = 3
};
inline std::string to_string(const ProType& proType) {
    switch (proType) {
        case ProType::NETWORK:
            return std::string("NETWORK");
        case ProType::LOAD_DB:
            return std::string("LOAD_DB");
        case ProType::ALGO:
            return std::string("ALGO");
        case ProType::LOAD_CSV:
            return std::string("LOAD_CSV");
    }
    return std::string("");
}

inline std::ostream& operator<<(std::ostream& os, const ProType& proType) {
    switch (proType) {
        case ProType::NETWORK:
            os << "NETWORK";
            break;
        case ProType::LOAD_DB:
            os << "LOAD_DB";
            break;
        case ProType::ALGO:
            os << "ALGO";
            break;
        case ProType::LOAD_CSV:
            os << "LOAD_CSV";
            break;
    }
    return os;
}
template <>
struct fmt::formatter<ProType> : formatter<string_view> {
    // 自定义格式化函数
    template <typename FormatContext>
    auto format(ProType pt, FormatContext& ctx) {
        string_view name;
        switch (pt) {
            case ProType::NETWORK:
                name = "NETWORK";
                break;
            case ProType::LOAD_DB:
                name = "LOAD_DB";
                break;
            case ProType::ALGO:
                name = "ALGO";
                break;
            case ProType::LOAD_CSV:
                name = "LOAD_CSV";
                break;
        }
        return formatter<string_view>::format(name, ctx);
    }
};
class TimeTrack
{
public:
    TimeTrack() = default;
    TimeTrack(ProType pro_type)
    {
        pro_type_ = pro_type;
    }
    ProType Type()
    {
        return pro_type_;
    }
    void Count(uint32_t diff)
    {
        times_.push_back(diff);
    }
    uint32_t Num() const
    {
        return times_.size();
    }
    uint32_t Avg() const
    {
        return Sum()/times_.size();
    }
    uint32_t Sum() const
    {
        return std::accumulate(times_.begin(), times_.end(), 0);
    }
        // 重载输出操作符 <<
    friend std::ostream& operator<<(std::ostream& os, const TimeTrack& tt) {
        os << "ProType: " << (tt.pro_type_) << std::endl;
        os << "Num: " << tt.Num() << std::endl;
        os << "Avg: " << tt.Avg() << std::endl;
        os << "Sum: " << tt.Sum() << std::endl;

        return os;
    }
private:
    ProType pro_type_;
    std::vector<uint32_t> times_;
};

template <>
struct fmt::formatter<TimeTrack> : formatter<string_view> {
    // 自定义格式化函数
    template <typename FormatContext>
    auto format(const TimeTrack& tt, FormatContext& ctx) {
        return formatter<string_view>::format(
            fmt::format("ProType: {} Num: {} Avg: {} Sum: {}",
                tt.pro_type_, tt.Num(), tt.Avg(), tt.Sum()), ctx);
    }
};

class TimeProfiler
{
public:
    TimeProfiler() = default;
    ~TimeProfiler();
    void Count(const std::string& profile_name,ProType pro_type);
    void Flush();
    void Print();
    void Disable(){ disabled_ = true;}
    std::string ProtoString();
private:
    std::map<std::string,TimeTrack> maps;
    std::string last_func;
    ProType last_pro_type;
    std::chrono::high_resolution_clock::time_point start_time;
    bool disabled_ =false;
};