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
enum class ProType
{
    NETWORK = 0,
    LOAD_DB = 1,
    ALGO =2,
    LOAD_CSV = 3
};
std::ostream& operator<<(std::ostream& os, const ProType& proType) {
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