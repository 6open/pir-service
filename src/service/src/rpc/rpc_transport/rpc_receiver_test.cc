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

#include <thread>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "rpc_key_adpapter.h"
#include "rpc_receiver.h"
using namespace mpc::rpc;

class RpcReceiverTest :  public ::testing::Test {};

TEST_F(RpcReceiverTest, simple_thead) {
    RpcReceiver rpc_receiver;
    RpcSeqSender seq_sender;
    rpc_receiver.SetRecvTimeout(1000);
    std::string key1("sahdj");
    std::string message1("dajhdjahkd");
    std::string build_key1 = seq_sender.BuildKey(key1);
    rpc_receiver.OnMessage(build_key1,message1);
    std::string recv_message1 = rpc_receiver.Recv(key1);
    EXPECT_EQ(recv_message1,message1);
    EXPECT_ANY_THROW(rpc_receiver.Recv(key1));
}

TEST_F(RpcReceiverTest, multi_simple_thead) {
    RpcReceiver rpc_receiver;
    RpcSeqSender seq_sender;
    int num = 10;
    std::thread onrecv_thread([&]()
    {
        for(int i=0;i<num;++i)
        {
            std::string key = std::to_string(i);
            std::string message(key);
            std::string build_key = seq_sender.BuildKey(key);
            std::cout<<"send message:"<<message<<std::endl;

            rpc_receiver.OnMessage(build_key,message);
        }
    });
    std::thread recv_thread([&]()
    {
        for(int i=0;i<num;++i)
        {
            std::string key = std::to_string(i);
            std::string message(key);
            std::string recv_message = rpc_receiver.Recv(key);
            std::cout<<"get message:"<<recv_message<<std::endl;
            // EXPECT_EQ(recv_message,message);
        }
    });
    onrecv_thread.join();
    recv_thread.join();
}