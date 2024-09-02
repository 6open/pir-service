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

#include "receiver_brpc_gateway.h"
#include "service_call.h"
#include "channel_brpc_gateway.h"
#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"

#include "spdlog/spdlog.h"

namespace yacl::link {

// namespace ic = org::interconnection;
// namespace ic_pb = org::interconnection::link;
namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;

namespace internal {
class GatewayProxyServiceImpl : public gateway_service::TransferService {
 public:
  explicit GatewayProxyServiceImpl(
      std::map<size_t, std::shared_ptr<IChannel>> listener)
      : listeners_(std::move(listener)) {
        ResigerServiceCall();
      }

  void ResigerServiceCall()
  {
    servicesCall_.insert({SPU_SERVICE_TYPE,std::make_unique<SpuServiceCall>(listeners_)});
  }

  void send(::google::protobuf::RpcController* /*cntl_base*/,
            const gateway_pb::TransferMeta* gateway_request, gateway_pb::ReturnStatus* gateway_response,
            ::google::protobuf::Closure* done) override {
    brpc::ClosureGuard done_guard(done);
    try {
        // SPDLOG_INFO("GatewayProxyServiceImpl  {}",gateway_request->DebugString());
        gateway_response->set_sessionid(gateway_request->sessionid());
        gateway_response->set_code(0);
        const gateway_pb::Content& content = gateway_request->content();
        ServiceCall* service = GetService(content.objectdata());
        if(service == nullptr){
          gateway_response->set_code(201);
          gateway_response->set_message("not support sevice:"+content.objectdata());
        } else {
          service->OnServiceOncall(content,*(gateway_response->mutable_content()));
        }
    } catch (const std::exception& e) {
        gateway_response->set_code(201);
        gateway_response->set_message(e.what());
    }
  }
  ServiceCall* GetService(const std::string& serviceType)
  {
    auto iter = servicesCall_.find(serviceType);
    if(iter != servicesCall_.end()){
      return iter->second.get();
    } else {
      return nullptr;
    }
  }
 protected:
  std::map<std::string,std::unique_ptr<ServiceCall>> servicesCall_;
  std::map<size_t, std::shared_ptr<IChannel>> listeners_;

};

}  // namespace internal

void ReceiverLoopBrpcGateway::StopImpl() {
  server_.Stop(0);
  server_.Join();
}

ReceiverLoopBrpcGateway::~ReceiverLoopBrpcGateway() { StopImpl(); }

void ReceiverLoopBrpcGateway::Stop() { StopImpl(); }

std::string ReceiverLoopBrpcGateway::Start(const std::string& host) {
  if (server_.IsRunning()) {
    YACL_THROW_LOGIC_ERROR("brpc server is already running");
  }

  auto svc = std::make_unique<internal::GatewayProxyServiceImpl>(listeners_);
  if (server_.AddService(svc.get(), brpc::SERVER_OWNS_SERVICE) == 0) {
    // Once add service succeed, give up ownership
    static_cast<void>(svc.release());
  } else {
    YACL_THROW_IO_ERROR("brpc server failed to add msg service");
  }

  // Start the server.
  brpc::ServerOptions options;
  if (server_.Start(host.data(), &options) != 0) {
    YACL_THROW_IO_ERROR("brpc server failed start");
  }

  return butil::endpoint2str(server_.listen_address()).c_str();
}

}  // namespace yacl::link

