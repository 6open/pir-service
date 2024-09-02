// Copyright 2019 Ant Group Co., Ltd.
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
#include <set>
#include <vector>
namespace mpc {
namespace rpc {

// const std::string kAckKey{'A', 'C', 'K', '\x01', '\x02'};
// const std::string kFinKey{'F', 'I', 'N', '\x01', '\x02'};
const std::string kSequenceKey{'\x01', '\x02'};
const std::string kAdpapterKey{'_','\x01', '\x02'};

class RpcKeyAdpapter
{
public:
    RpcKeyAdpapter(const std::string& key);
    ~RpcKeyAdpapter() = default;
    std::string Mask(const std::string& info);
    std::string Key() const {return key_;};
    static std::pair<std::string,std::string> UnMask(const std::string& info);
private:
    std::string real_key_;
    std::string key_;
    // std::string kAdpapterKey;
};

//not thread safe ,should protected by the caller
class RpcSeqReceiver
{
public:
    RpcSeqReceiver();
    bool Insert(std::size_t sequence);
    template <class View>
    static std::size_t ViewToSizeT(View v);
    static std::pair<std::string, size_t> SplitKey(std::string_view key);
private:

    std::set<std::size_t> received_msg_ids_;
    // std::vector<bool> received_msg_ids_;
};

class RpcSeqSender
{
public:
    // RpcSeqSender();
    std::string BuildKey(std::string_view msg_key);
    std::size_t Seq() const {return sequence_id_;};
private:
    //begin from 1
    std::size_t sequence_id_ = 1;
};

} // namespace rpc
} // namespace mpc