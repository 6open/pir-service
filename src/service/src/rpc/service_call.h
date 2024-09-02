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
// #include "channel_brpc_gateway.h"
#include <map>
#include "yacl/link/transport/channel.h"

#include "interconnection/link/transport.pb.h"
#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"

namespace yacl::link {

namespace ic = org::interconnection;
namespace ic_pb = org::interconnection::link;
namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;
class ServiceCall
{
public:
    virtual void OnServiceOncall(const gateway_pb::Content& request_content,gateway_pb::ResponseContent& reponse_content) = 0;
    virtual ~ServiceCall() = default;
// private:
};

class SpuServiceCall : public ServiceCall
{
public:
    SpuServiceCall(std::map<size_t, std::shared_ptr<IChannel>> listeners)
    :ServiceCall(),listeners_(std::move(listeners))
    {

    }
    void OnServiceOncall(const gateway_pb::Content& request_content,gateway_pb::ResponseContent& reponse_content) override;
private:
  void OnRpcCall(size_t src_rank, const std::string& key,
                 const std::string& value);

  void OnRpcCall(size_t src_rank, const std::string& key,
                 const std::string& value, size_t offset, size_t total_length);
private:
  std::map<size_t, std::shared_ptr<IChannel>> listeners_;
};

}  // namespace yacl::link
