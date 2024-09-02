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

#include "channel_brpc_gateway.h"
#include "receiver_brpc_gateway.h"
#include "gateway_proxy_mock.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <future>
#include <string>

#include "fmt/format.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "yacl/base/byte_container_view.h"
#include "yacl/base/exception.h"

#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"


// namespace ic = org::interconnection;
// namespace ic_pb = org::interconnection::link;
using namespace yacl::link;
namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;

class EndpointServiceImpl : public gateway_service::TransferService {
public:
    void send(::google::protobuf::RpcController* /*cntl_base*/,
            const gateway_pb::TransferMeta* gateway_request, gateway_pb::ReturnStatus* gateway_response,
            ::google::protobuf::Closure* done) override {
    brpc::ClosureGuard done_guard(done);
    gateway_response->set_sessionid(gateway_request->sessionid());
    gateway_response->set_code(200);
    const gateway_pb::Content& content = gateway_request->content();
    gateway_response->mutable_content()->set_objectbytedata(content.objectbytedata());
    gateway_response->mutable_content()->set_objectdata(content.objectdata());
    }
};
class GateProxyTest : public ::testing::Test {
 protected:
  void SetUp() override {
    std::srand(std::time(nullptr));
    proxy_host_ = "127.0.0.1:10000";
    receiver_host_ = "127.0.0.1:10001";
    send_host_ = "127.0.0.1:10002";
    end_host_ = "127.0.0.1:10003";

    end_member_id_ = "end_member_id";
    const size_t send_rank = 0;
    const size_t recv_rank = 1;
    send_member_id_ = "send_member";
    recv_member_id_ = "recv_member";
    options1_.self_member_id = send_member_id_;
    options1_.peer_member_id = recv_member_id_;
    options2_.self_member_id = recv_member_id_;
    options2_.peer_member_id = send_member_id_;
    sender_ = std::make_shared<ChannelBrpcGateWay>(send_rank, recv_rank, options1_);
    receiver_ = std::make_shared<ChannelBrpcGateWay>(recv_rank, send_rank, options2_);

    // let sender rank as 0, receiver rank as 1.
    // receiver_ listen messages from sender(rank 0).
    receiver_loop_ = std::make_unique<ReceiverLoopBrpcGateway>();
    receiver_loop_->AddListener(0, receiver_);
    receiver_host_ = receiver_loop_->Start(receiver_host_);

    sender_loop_ = std::make_unique<ReceiverLoopBrpcGateway>();
    sender_loop_->AddListener(1, sender_);
    sender_host_ = sender_loop_->Start(send_host_);

    //


    sender_->SetPeerHost(proxy_host_);
    receiver_->SetPeerHost(send_host_);
    brpc::ServerOptions options;
    auto svc1 = std::make_unique<GatewayProxyServiceMock>();
    svc1->RegisterMember(recv_member_id_,receiver_host_);
    svc1->RegisterMember(end_member_id_,end_host_);

    if (proxy_server_.AddService(svc1.get(), brpc::SERVER_OWNS_SERVICE) == 0) {
        // Once add service succeed, give up ownership
        static_cast<void>(svc1.release());
    } else {
        YACL_THROW_IO_ERROR("brpc server failed to add msg service");
    }
    if (proxy_server_.Start(proxy_host_.data(), &options) != 0) {
        YACL_THROW_IO_ERROR("brpc server failed start");
    }

    auto svc2 = std::make_unique<EndpointServiceImpl>();
    if (end_server_.AddService(svc2.get(), brpc::SERVER_OWNS_SERVICE) == 0) {
        // Once add service succeed, give up ownership
        static_cast<void>(svc2.release());
    } else {
        YACL_THROW_IO_ERROR("brpc server failed to add msg service");
    }
    if (end_server_.Start(end_host_.data(), &options) != 0) {
        YACL_THROW_IO_ERROR("brpc server failed start");
    }
  }

  void TearDown() override {
    auto wait = [](std::shared_ptr<ChannelBrpcGateWay>& l) {
      if (l) {
        l->WaitLinkTaskFinish();
      }
    };
    auto f_s = std::async(wait, std::ref(sender_));
    auto f_r = std::async(wait, std::ref(receiver_));
    f_s.get();
    f_r.get();

    proxy_server_.Stop(0);
    proxy_server_.Join();
    end_server_.Stop(0);
    end_server_.Join();
  }

  ChannelBrpcGateWay::Options options1_;
  ChannelBrpcGateWay::Options options2_;
  std::string send_member_id_;
  std::string recv_member_id_;
  std::string end_member_id_;

  std::shared_ptr<ChannelBrpcGateWay> sender_;
  std::shared_ptr<ChannelBrpcGateWay> receiver_;
  std::string receiver_host_;
  std::unique_ptr<ReceiverLoopBrpcGateway> receiver_loop_;
  std::string sender_host_;
  std::unique_ptr<ReceiverLoopBrpcGateway> sender_loop_;

  brpc::Server proxy_server_;
  brpc::Server end_server_;
  std::string proxy_host_;
  std::string send_host_;
  std::string end_host_;
};


TEST_F(GateProxyTest, Normal_Empty) {
  std::cout<<"hello_______________"<<std::endl;
  const std::string key = "key";
  const std::string sent("hdsfghdsg");
  sender_->SendAsync(key, yacl::ByteContainerView{sent});
  auto received = receiver_->Recv(key);

  EXPECT_EQ(sent, std::string_view(received));
}

TEST_F(GateProxyTest, SendByte) {
    std::cout<<"GateProxyTest.SendByte___________________________________________"<<std::endl;
    auto brpc_channel = std::make_unique<brpc::Channel>();
    const auto load_balancer = "";
    brpc::ChannelOptions options;
    {
        options.protocol = "h2:grpc";
        options.connection_type = "single";
        options.connect_timeout_ms = 20000;
        options.timeout_ms = 60*1000;
        options.max_retry = 3;
        // options.retry_policy = DefaultRpcRetryPolicy();
    }
    int res = brpc_channel->Init(proxy_host_.c_str(), load_balancer, &options);
    if (res != 0) {
        YACL_THROW_NETWORK_ERROR("Fail to initialize channel, host={}, err_code={}",
                                proxy_host_, res);
    }

    brpc::Controller cntl;
    //gateway_pb::ReturnStatus gateway_response;
    gateway_service::TransferService::Stub stub(brpc_channel.get());

    gateway_pb::TransferMeta gateway_request;
    std::string sessionid("fsdhkjfds");
    std::string object_type("hdsadgajs");
    std::string object_data("ashdsah");
    gateway_request.set_sessionid(sessionid);
    gateway_request.mutable_src()->set_memberid(send_member_id_);
    gateway_request.mutable_dst()->set_memberid(end_member_id_);
    gateway_request.mutable_content()->set_objectdata(object_type);
    gateway_request.mutable_content()->set_objectbytedata(object_data);

    gateway_pb::ReturnStatus gateway_response;
    stub.send(&cntl, &gateway_request, &gateway_response, nullptr);
    std::cout<<"get reponse:"<<gateway_response.sessionid()<<std::endl;
    EXPECT_EQ(sessionid,gateway_response.sessionid());
    EXPECT_EQ(object_data,gateway_response.content().objectbytedata());
    EXPECT_EQ(object_type,gateway_response.content().objectdata());
    
}

/*
bazel test //src/rpc:gateway_proxy_test --test_filter=GateProxyTest.Test98gateway
*/
TEST_F(GateProxyTest, Test98gateway) {
    auto brpc_channel = std::make_unique<brpc::Channel>();
    const auto load_balancer = "";
    brpc::ChannelOptions options;
    {
        options.protocol = "h2:grpc";
        options.connection_type = "single";
        options.connect_timeout_ms = 20000;
        options.timeout_ms = 60*1000;
        options.max_retry = 3;
        // options.retry_policy = DefaultRpcRetryPolicy();
    }
    int res = brpc_channel->Init("192.168.10.98:12000", load_balancer, &options);
    if (res != 0) {
        throw std::runtime_error("Fail to initialize channel, host=192.168.10.98:12000, err_code={}"  + std::to_string(res));
    }
  gateway_pb::TransferMeta gateway_request;
  gateway_request.set_sessionid("asaa");
  gateway_request.set_processor("gatewayAliveProcessor");
  gateway_request.mutable_src()->set_memberid("bc87248609e14721a11fab88f7269ab7");
  gateway_request.mutable_dst()->set_memberid("dce7c138a65c433f926eb42a4c12115f");
  gateway_request.mutable_content()->set_objectdata(SPU_SERVICE_TYPE);
  gateway_request.mutable_content()->set_objectbytedata("hello");

  brpc::Controller cntl;
  gateway_pb::ReturnStatus gateway_response;
  gateway_service::TransferService::Stub stub(brpc_channel.get());
  stub.send(&cntl, &gateway_request, &gateway_response, nullptr);
  if (cntl.Failed()) {
    std::cout<<"ChannelBrpcGateWay send, rpc failed={}, message={}"<<
                             cntl.ErrorCode()<< cntl.ErrorText()<<std::endl;
  }
  EXPECT_EQ(false,cntl.Failed());
  std::cout<<"reponse "<<gateway_response.DebugString()<<std::endl;
}
