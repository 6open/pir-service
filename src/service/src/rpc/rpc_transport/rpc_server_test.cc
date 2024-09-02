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

#include "rpc_server.h"
#include "rpc_session.h"

using namespace mpc::rpc;

class RpcServerTest :  public ::testing::Test {};

/*
bazel test //src/rpc/rpc_transport:rpc_server_test --test_filter=RpcServerTest.normal_test
*/
TEST_F(RpcServerTest, normal_test)
{
    RpcServer rpc_server;
    uint16_t port = 12300;
    std::string host = "127.0.0.1:"+std::to_string(port);
    rpc_server.Start(host);
    std::cout<<"server started"<<std::endl;
    GatewayPrama parma;
    std::string member1("member1");
    std::string member2("member2");
    std::string taskid("taskid");
    parma.self_memberid = member1;
    parma.peer_memberid = member2;
    parma.session_id = "session_id";
    parma.task_id = taskid;
    RpcSession rpc_session(&rpc_server,host,parma);
    rpc_session.SetRecvTimeout(1000);

    // auto recver = rpc_server.GetReceiver(session_key);
    // EXPECT_TRUE(recver!=nullptr);

    GatewayPrama parma2;
    parma2.self_memberid = member2;
    parma2.peer_memberid = member1;
    parma2.session_id = "session_id";
    parma2.task_id = taskid;

    RpcSession rpc_session2(&rpc_server,host,parma2);
    rpc_session2.SetRecvTimeout(1000);


    EXPECT_ANY_THROW(rpc_session.Recv("haha"));
    rpc_session.Send("haha","hsshhs");
    EXPECT_EQ(rpc_session2.Recv("haha"),"hsshhs");
    EXPECT_ANY_THROW(rpc_session2.Recv("haha"));

    // rpc_session2.Recv("haha");
    std::cout<<"rpc_session.Recv"<<std::endl;

    rpc_session2.Send("hhhh","shdhd");
    EXPECT_EQ(rpc_session.Recv("hhhh"),"shdhd");
    EXPECT_ANY_THROW(rpc_session.Recv("hhhh"));

    std::string lenmsg(40,'a');
    rpc_session2.Send("len",lenmsg);
    EXPECT_EQ(rpc_session.Recv("len"),lenmsg);
}

TEST_F(RpcServerTest, multi_thread)
{
    RpcServer rpc_server;
    uint16_t port = 12300;
    std::string host = "127.0.0.1:"+std::to_string(port);
    rpc_server.Start(host);
    std::string base_message("dskhfhkjfds");
    auto funcSend = [&rpc_server,&base_message,&host](int i)
    {
        GatewayPrama parma;
        std::string member1 = "member"+std::to_string(i);
        std::string member2 = "member"+std::to_string(i+10);
        std::string taskid("taskid");
        parma.self_memberid = member1;
        parma.peer_memberid = member2;
        parma.session_id = "session_id";
        parma.task_id = taskid;
        RpcSession rpc_session(&rpc_server,host,parma);
        std::string send_msg = base_message+std::to_string(i);
        rpc_session.Send("haha",send_msg);
    };
    auto funcRecv= [&rpc_server,&base_message,&host](int i)
    {
        GatewayPrama parma;
        std::string member1 = "member"+std::to_string(i);
        std::string member2 = "member"+std::to_string(i+10);
        std::string taskid("taskid");
        parma.self_memberid = member2;
        parma.peer_memberid = member1;
        parma.session_id = "session_id";
        parma.task_id = taskid;
        RpcSession rpc_session(&rpc_server,host,parma);
        std::string expect_recv_msg = base_message+std::to_string(i);
        std::string ret = rpc_session.Recv("haha");
        EXPECT_EQ(expect_recv_msg,ret);
    };

    std::vector<std::thread> send_threads;
    std::vector<std::thread> recv_threads;
    std::size_t thread_num = 10;
    for(std::size_t i=0;i<thread_num;++i)
    {
        send_threads.emplace_back(std::thread(funcSend,i));
        recv_threads.emplace_back(std::thread(funcRecv,i));
    }
    for(std::size_t i=0;i<thread_num;++i)
    {
        send_threads[i].join();
        recv_threads[i].join();
    }
}

TEST_F(RpcServerTest, test_two_server)
{
    RpcServer rpc_server1;
    RpcServer rpc_server2;

    uint16_t port1 = 12301;
    uint16_t port2 = 12302;

    std::string host1 = "127.0.0.1:"+std::to_string(port1);
    std::string host2 = "127.0.0.1:"+std::to_string(port2);

    rpc_server1.Start(host1);
    rpc_server2.Start(host2);

    std::string base_message("ajsdkaksd");
    auto func1([&rpc_server1,&rpc_server2,&host1,&host2,&base_message](int i)
    {
        GatewayPrama parma;
        std::string member1 = "member"+std::to_string(i);
        std::string member2 = "member"+std::to_string(i+10);
        std::string taskid("taskid");
        parma.self_memberid = member1;
        parma.peer_memberid = member2;
        RpcSession rpc_session(&rpc_server1,host2,parma);
        std::string send_msg = base_message+std::to_string(i);
        rpc_session.Send("haha",send_msg);
        std::string expect_recv_msg = base_message+std::to_string(i+101);
        std::string ret = rpc_session.Recv("haha");
        EXPECT_EQ(expect_recv_msg,ret);
    });
    auto func2([&rpc_server1,&rpc_server2,&host1,&port2,&base_message](int i)
    {
        GatewayPrama parma;
        std::string member1 = "member"+std::to_string(i);
        std::string member2 = "member"+std::to_string(i+10);
        std::string taskid("taskid");
        parma.self_memberid = member2;
        parma.peer_memberid = member1;
        RpcSession rpc_session(&rpc_server2,host1,parma);
        std::string send_msg = base_message+std::to_string(i+101);
        std::string expect_recv_msg = base_message+std::to_string(i);
        std::string ret = rpc_session.Recv("haha");
        EXPECT_EQ(expect_recv_msg,ret);
        rpc_session.Send("haha",send_msg);
    });
    std::vector<std::thread> send_threads;
    std::vector<std::thread> recv_threads;
    std::size_t thread_num = 10;
    for(std::size_t i=0;i<thread_num;++i)
    {
        send_threads.emplace_back(std::thread(func1,i));
        recv_threads.emplace_back(std::thread(func2,i));
    }
    for(std::size_t i=0;i<thread_num;++i)
    {
        send_threads[i].join();
        recv_threads[i].join();
    }
}

/*
bazel test //src/rpc/rpc_transport:rpc_server_test --test_filter=RpcServerTest.big_data
*/
TEST_F(RpcServerTest, big_data)
{
    RpcServer rpc_server;
    uint16_t port = 12300;
    std::string host = "127.0.0.1:"+std::to_string(port);
    rpc_server.Start(host);
    std::cout<<"server started"<<std::endl;
    GatewayPrama parma;
    std::string member1("member1");
    std::string member2("member2");
    std::string taskid("taskid");
    parma.self_memberid = member1;
    parma.peer_memberid = member2;
    parma.session_id = "session_id";
    parma.task_id = taskid;
    RpcSession rpc_session(&rpc_server,host,parma);
    // rpc_session.SetRecvTimeout(1000);

    // auto recver = rpc_server.GetReceiver(session_key);
    // EXPECT_TRUE(recver!=nullptr);

    GatewayPrama parma2;
    parma2.self_memberid = member2;
    parma2.peer_memberid = member1;
    parma2.session_id = "session_id";
    parma2.task_id = taskid;

    RpcSession rpc_session2(&rpc_server,host,parma2);
    rpc_session2.SetRecvTimeout(1000);


    rpc_session.Send("haha","hsshhs");
    EXPECT_EQ(rpc_session2.Recv("haha"),"hsshhs");
    std::cout<<"test send "<<std::endl;

    std::string lenmsg(64 * 1024 * 20,'a');
    std::cout<<"begin send "<<std::endl;
    rpc_session.Send("len",lenmsg);
    std::cout<<"end send "<<std::endl;

    EXPECT_EQ(rpc_session2.Recv("len"),lenmsg);
}