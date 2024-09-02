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

#include <iostream>
#include "rpc_key_adpapter.h"
#include "absl/strings/numbers.h"
#include "yacl/base/exception.h"    

namespace mpc {
namespace rpc {
// const std::string kAdpapterKey{'_','\x01', '\x02'};

RpcKeyAdpapter::RpcKeyAdpapter(const std::string& key)
{
    real_key_ = key;
    // std::hash<std::string> hasher;
    // std::size_t hash_value = hasher(real_key_);
    // key_ = std::to_string(hash_value);
    key_ = real_key_;
}

std::string RpcKeyAdpapter::Mask(const std::string& str)
{
    return Key()+kAdpapterKey+str;
}

std::pair<std::string,std::string> RpcKeyAdpapter::UnMask(const std::string& str)
{
    std::size_t found =str.find(kAdpapterKey);
    if(found != std::string::npos) {
        return std::make_pair(str.substr(0, found),str.substr(found + kAdpapterKey.size()));
    }
    else {
        return std::make_pair(str,std::string());
    }
}


RpcSeqReceiver::RpcSeqReceiver()
{
    // received_msg_ids_.reserve(1024);
}

bool RpcSeqReceiver::Insert(std::size_t sequence)
{
    return received_msg_ids_.insert(sequence).second;
}

template <class View>
size_t RpcSeqReceiver::ViewToSizeT(View v) 
{
  size_t ret = 0;
  YACL_ENFORCE(absl::SimpleAtoi(
      absl::string_view(reinterpret_cast<const char*>(v.data()), v.size()),
      &ret));
  return ret;
}

std::pair<std::string, size_t> RpcSeqReceiver::SplitKey(std::string_view key) 
{
  auto pos = key.find(kSequenceKey);
//   SPDLOG_INFO("SplitKey key:{}, pos:{},kSequenceKey:{}",key,pos,kSequenceKey);
//   std::cout<<"SplitKey key:"<<key<<", pos:"<<pos<<", kSequenceKey:"<<kSequenceKey<<std::endl;

  std::pair<std::string, std::size_t> ret;
  ret.first = key.substr(0, pos);
  ret.second = ViewToSizeT(key.substr(pos + kSequenceKey.size()));
//   std::cout<<"ret.first:"<<ret.first<<", ret.second:"<<ret.second<<", kSequenceKey:"<<kSequenceKey.size()<<std::endl;
  return ret;
}


std::string RpcSeqSender::BuildKey(std::string_view msg_key)
{
  return std::string(msg_key) + kSequenceKey + std::to_string(sequence_id_++);
}

} // namespace rpc
} // namespace mpc