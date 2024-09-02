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
#include "rpc_sender.h"
using namespace mpc::rpc;

class RpcSenderTest :  public ::testing::Test {};

TEST_F(RpcSenderTest, normal_test)
{
    std::string key("dsajdahj");
    RpcSender::Options options;
    GatewayPrama parma;
    parma.peer_memberid = "peer_memberid";
    parma.self_memberid = "self_memberid";
    parma.session_id = "session_id";
    RpcGatewaySender sender(key,options,parma);
    sender.SetPeerHost("127.0.0.1:12345");
    std::cout<<sender.GeneratorKey("haha")<<std::endl;
    EXPECT_ANY_THROW(sender.Send("haha","aaaa"));
}