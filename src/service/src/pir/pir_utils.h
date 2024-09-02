// Copyright 2019 Ant Group Co., Ltd
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
#include <string_view>

constexpr std::string_view kPirAlgoSpu = "SPU";
constexpr std::string_view kPirAlgoSe = "SE";

constexpr uint32_t kLinkRecvTimeout =  30*60 * 1000;

enum class PirType
{
    UNKNOW = 0,
    SPU = 1,
    SE = 2
};


inline PirType GetPirType(const std::string_view algo,const std::string& default_algo = "SE")
{
    if(algo.empty()){
        if(default_algo == kPirAlgoSe)
            return PirType::SE;
        else if(default_algo == kPirAlgoSpu)
            return PirType::SPU;
        else
            return PirType::SE;
    } else if(algo == kPirAlgoSpu)
        return PirType::SPU;
    else if(algo == kPirAlgoSe)
        return PirType::SE;
    return PirType::UNKNOW;
}