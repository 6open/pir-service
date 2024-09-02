
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
#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"
#include "brpc/channel.h"




// using namespace yacl::link;
namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;

class GatewayProxyServiceMock : public gateway_service::TransferService {
public:
    void send(::google::protobuf::RpcController* /*cntl_base*/,
            const gateway_pb::TransferMeta* gateway_request, gateway_pb::ReturnStatus* gateway_response,
            ::google::protobuf::Closure* done) override ;

  void RegisterMember(const std::string& member_id,const std::string& peer_host)
  {
    member_maps_[member_id] = peer_host;
  }
private:
    std::map<std::string,std::string> member_maps_;
};