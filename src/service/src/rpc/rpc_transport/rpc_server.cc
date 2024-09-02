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
#include "rpc_server.h"
#include "rpc_key_adpapter.h"
#include "src/rpc/proto/basic-meta.pb.h"
#include "src/rpc/proto/gateway-meta.pb.h"
#include "src/rpc/proto/gateway-service.pb.h"
#include "src/http_server/proto/http.pb.h"
#include "spdlog/spdlog.h"
#include "yacl/base/exception.h"    

namespace mpc {
namespace rpc {

namespace gateway_service =  com::nvxclouds::apollo::gateway::api::service::proto;
namespace gateway_pb = com::nvxclouds::apollo::gateway::api::meta::basic;

using namespace example;

class HealthCheckServiceImpl : public HttpService {
public:
    HealthCheckServiceImpl() {}
    virtual ~HealthCheckServiceImpl() {}
    void Echo(google::protobuf::RpcController* cntl_base,
              const HttpRequest*,
              HttpResponse*,
              google::protobuf::Closure* done) {
        // This object helps you to call done->Run() in RAII style. If you need
        // to process the request asynchronously, pass done_guard.release().
        brpc::ClosureGuard done_guard(done);
        
        brpc::Controller* cntl =
            static_cast<brpc::Controller*>(cntl_base);
        // Fill response.
        cntl->http_response().set_content_type("text/plain");
        butil::IOBufBuilder os;
        os << "queries:";
        for (brpc::URI::QueryIterator it = cntl->http_request().uri().QueryBegin();
                it != cntl->http_request().uri().QueryEnd(); ++it) {
            os << ' ' << it->first << '=' << it->second;
        }
        os << "\nbody: " << cntl->request_attachment() << '\n';
        os.move_to(cntl->response_attachment());
    }
};

class GatewayProxyServiceImpl : public gateway_service::TransferService {
 public:
  using MessageCallback = std::function<void(const std::string& key,const std::string& message,std::map<std::string, std::string>& msg_config ,std::string& errorInfo)>;
  explicit GatewayProxyServiceImpl(const MessageCallback& callback) 
    :message_callback_(callback)
    {
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
        std::map<std::string, std::string> msg_config;
        for(int i=0;i<content.configdatas_size();++i){
          msg_config.insert(std::make_pair(content.configdatas(i).key(),content.configdatas(i).value()));
        }
        std::string errorMessage;
        message_callback_(content.objectdata(),content.objectbytedata(),msg_config,errorMessage);
        // SPDLOG_INFO("message_callback_1 key:{},message:{}",content.objectdata(),content.objectbytedata());

        if(!errorMessage.empty()) {
            gateway_response->set_code(201);
            gateway_response->set_message(errorMessage);
        }
        // SPDLOG_INFO("message_callback_2 key:{},message:{}",content.objectdata(),content.objectbytedata());

    } catch (const std::exception& e) {
        gateway_response->set_code(201);
        gateway_response->set_message(e.what());
    }
  }

 protected:
  MessageCallback message_callback_;
};


RpcServer::RpcServer()
{

}

RpcServer::~RpcServer()
{
  StopImpl();
}

void RpcServer::StopImpl() 
{
  server_.Stop(0);
  server_.Join();
}

void RpcServer::Stop()
{
  StopImpl();
}

std::string RpcServer::Start(const std::string& host)
{
  if (server_.IsRunning()) {
    YACL_THROW_LOGIC_ERROR("brpc server is already running");
  }

  HealthCheckServiceImpl http_svc;
  if (server_.AddService(&http_svc,
                          brpc::SERVER_DOESNT_OWN_SERVICE) != 0) {
        LOG(ERROR) << "Fail to add http_svc";
        return "fuck";
    }
  auto svc = std::make_unique<GatewayProxyServiceImpl>(std::bind(&RpcServer::OnMessage,this,std::placeholders::_1,std::placeholders::_2,std::placeholders::_3,std::placeholders::_4));
  if (server_.AddService(svc.get(), brpc::SERVER_OWNS_SERVICE) == 0) {
    // Once add service succeed, give up ownership
    static_cast<void>(svc.release());
  } else {
    YACL_THROW_IO_ERROR("brpc server failed to add msg service");
  }

  // Start the server.
  brpc::ServerOptions options;
  options.h2_settings.stream_window_size = 256 * 1024*1024;
  options.h2_settings.connection_window_size = 1024 * 1024*1024;
  if (server_.Start(host.data(), &options) != 0) {
    YACL_THROW_IO_ERROR("brpc server failed start");
  }

  return butil::endpoint2str(server_.listen_address()).c_str();
}

std::shared_ptr<RpcReceiver> RpcServer::AddReceiver(const std::string& key)
{
    std::unique_lock<bthread::Mutex> lock(receiver_mutex_);
    auto iter = receivers_.find(key);
    if(iter != receivers_.end())
    {        
        SPDLOG_WARN("Receiver exit, key={}",key);
        return iter->second;
    }
    else
    {
        std::shared_ptr<RpcReceiver> lis = std::make_shared<RpcReceiver>(key);
        receivers_.insert(std::make_pair(key,lis));
        return lis;
    }
}

std::shared_ptr<RpcReceiver> RpcServer::GetReceiver(const std::string& key)
{
    std::unique_lock<bthread::Mutex> lock(receiver_mutex_);
    auto iter = receivers_.find(key);
    if(iter != receivers_.end())
        return iter->second;
    else
        return nullptr;
}

void RpcServer::RemoveReceiver(const std::string& key)
{
    std::unique_lock<bthread::Mutex> lock(receiver_mutex_);
    SPDLOG_INFO("RpcServer::RemoveReceiver, key={}",key);
    receivers_.erase(key);
}


void RpcServer::OnMessage(const std::string& key,const std::string& message,std::map<std::string, std::string>& msg_config,std::string& errorInfo)
{
    std::pair<std::string,std::string> key_info = RpcKeyAdpapter::UnMask(key);
    if(key_info.first.empty() || key_info.second.empty()) {
        errorInfo = "key need is XXX_XXX, sub is empty: "+key;
        return;
    }
    auto iter = msg_config.find("msgtype");
    if(iter != msg_config.end())
    {
      if(iter->second == "chunked"){
        std::string len_str = msg_config["len"];
        std::string offset_str = msg_config["offset"];
        std::size_t offset = 0;
        size_t total_length = 0;
        if(absl::SimpleAtoi(absl::string_view(reinterpret_cast<const char*>(len_str.data()), len_str.size()),&total_length) == false) {
          errorInfo = "chunked msg  len cannot convert to int , it is :"+len_str;
          return;
        }
        if(absl::SimpleAtoi(absl::string_view(reinterpret_cast<const char*>(offset_str.data()), offset_str.size()),&offset) == false) {
          errorInfo = "chunked msg  offset cannot convert to int , it is :"+offset_str;
          return;
        }
        auto lis = GetReceiver(key_info.first);
        if(lis == nullptr)
          lis = AddReceiver(key_info.first);
        lis->OnChunkedMessage(key_info.second,message,offset,total_length);
        return;
      }
      else if(iter->second == "normal") {
        auto lis = GetReceiver(key_info.first);
        if(lis == nullptr)
          lis = AddReceiver(key_info.first);
        lis->OnMessage(key_info.second,message,msg_config);
        return;
      } else {
        errorInfo = "msgtype not int chunked and normal , it is :"+iter->second;
        return;
      }
    } else {
        errorInfo = "msgtype not set";
        return;
    }
    // SPDLOG_INFO("OnMessage key:{},message:{}",key,message);
}

} // namespace rpc
} // namespace mpc