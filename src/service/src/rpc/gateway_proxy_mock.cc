
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

#include "gateway_proxy_mock.h"

void GatewayProxyServiceMock::send(::google::protobuf::RpcController* /*cntl_base*/,
            const gateway_pb::TransferMeta* gateway_request, gateway_pb::ReturnStatus* gateway_response,
            ::google::protobuf::Closure* done) 
{
    brpc::ClosureGuard done_guard(done);
    std::cout<<"GatewayProxyServiceMock recv session id="<<gateway_request->sessionid()<<std::endl;
    std::cout<<"GatewayProxyServiceMock recv content ="<<gateway_request->content().objectbytedata()<<std::endl;
    std::string dst_member_id = gateway_request->dst().memberid();
    if(member_maps_.find(dst_member_id) == member_maps_.end())
    {
        std::cout<<"dst_member_id:"<<dst_member_id<<" not exited"<<std::endl;
        return;
    }
    std::string peer_host = member_maps_[dst_member_id];
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
    int res = brpc_channel->Init(peer_host.c_str(), load_balancer, &options);
    if (res != 0) {
        throw std::runtime_error("Fail to initialize channel, host={}, err_code={}" +peer_host + std::to_string(res));
    }

    brpc::Controller cntl;
    gateway_service::TransferService::Stub stub(brpc_channel.get());
    stub.send(&cntl, gateway_request, gateway_response, nullptr);
}