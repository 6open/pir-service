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

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "rpc_key_adpapter.h"
using namespace mpc::rpc;

class RpcKeyAdpapterTest :  public ::testing::Test {};

TEST_F(RpcKeyAdpapterTest, RpcKeyAdpapter_Test) {
    std::string str("hello_workd");
    RpcKeyAdpapter key_adp(str);
    auto pos = key_adp.Key().find('_');
    EXPECT_EQ(pos,std::string::npos);
    std::string message("hiasdl_sadhk");

    auto masked = key_adp.Mask(message);
    auto ret = key_adp.UnMask(masked);
    EXPECT_EQ(key_adp.Key(),ret.first);
    EXPECT_EQ(message,ret.second);
}

TEST_F(RpcKeyAdpapterTest, RpcSeqReceiver_Test) {
    
    RpcSeqReceiver seq_receiver;
    EXPECT_TRUE(seq_receiver.Insert(1));
    EXPECT_TRUE(seq_receiver.Insert(2));
    EXPECT_TRUE(seq_receiver.Insert(3));
    EXPECT_TRUE(seq_receiver.Insert(421));
    EXPECT_TRUE(seq_receiver.Insert(23));

    EXPECT_FALSE(seq_receiver.Insert(1));
    EXPECT_FALSE(seq_receiver.Insert(2));
    EXPECT_FALSE(seq_receiver.Insert(3));
    EXPECT_FALSE(seq_receiver.Insert(421));
    EXPECT_FALSE(seq_receiver.Insert(23));

    RpcSeqSender seq_sender;
    std::string st;
    std::pair<std::string, size_t> pa;
    EXPECT_EQ(seq_sender.Seq(),1);
    st = seq_sender.BuildKey("hahah");
    pa = seq_receiver.SplitKey(st);
    EXPECT_EQ(pa.first,"hahah");
    EXPECT_EQ(pa.second,1);
    EXPECT_EQ(seq_sender.Seq(),2);
    st = seq_sender.BuildKey("hahah");
    pa = seq_receiver.SplitKey(st);
    EXPECT_EQ(pa.first,"hahah");
    EXPECT_EQ(pa.second,2);
    EXPECT_EQ(seq_sender.Seq(),3);
    st = seq_sender.BuildKey("hahah");
    pa = seq_receiver.SplitKey(st);
    EXPECT_EQ(pa.first,"hahah");
    EXPECT_EQ(pa.second,3);
    EXPECT_EQ(seq_sender.Seq(),4);
    EXPECT_ANY_THROW(seq_receiver.SplitKey("hahah"));
    st = "dahkj"+kSequenceKey+"jhdsja";
    EXPECT_ANY_THROW(seq_receiver.SplitKey(st));

}

TEST_F(RpcKeyAdpapterTest, Adp_And_SEQ_Test) {
    std::string str("hello_workd");
    RpcKeyAdpapter key_adp(str);
    RpcSeqSender seq_sender;
    RpcSeqReceiver seq_receiver;

    std::string ret = key_adp.Mask(seq_sender.BuildKey("hahah"));
    std::cout<<"ret is "<<ret<<std::endl;
    std::string umask = key_adp.UnMask(ret).second;
    std::cout<<"umask is "<<umask<<std::endl;
    std::pair<std::string, size_t> pa;
    pa = seq_receiver.SplitKey(umask);
    std::string hah = pa.first;
    std::cout<<"hah is "<<hah<<std::endl;
    std::cout<<"seq is "<<pa.second<<std::endl;
    std::string msg_key;
    size_t seq_id = 0;
    std::tie(msg_key, seq_id)  =  seq_receiver.SplitKey(umask);
    std::cout<<"msg_key is "<<msg_key<<std::endl;
    std::cout<<"seq_id is "<<seq_id<<std::endl;
}