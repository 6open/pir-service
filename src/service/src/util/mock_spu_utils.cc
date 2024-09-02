// Copyright 2021 Ant Group Co., Ltd.
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

#include "mock_spu_utils.h"

#include "absl/strings/match.h"
#include "absl/strings/str_split.h"

#include "libspu/spu.pb.h"

#include "src/rpc/factory_brpc_gateway.h"

namespace mpc{
namespace utils_fse{
// llvm::cl::opt<std::string> Parties(
//     "parties", llvm::cl::init("127.0.0.1:9530,127.0.0.1:9531"),
//     llvm::cl::desc("server list, format: host1:port1[,host2:port2, ...]"));
// llvm::cl::opt<uint32_t> Rank("rank", llvm::cl::init(0),
//                              llvm::cl::desc("self rank"));
llvm::cl::opt<uint32_t> ProtocolKind(
    "protocol_kind", llvm::cl::init(2),
    llvm::cl::desc("1 for REF2k, 2 for SEMI2k, 3 for ABY3, 4 for Cheetah"));
llvm::cl::opt<uint32_t> Field(
    "field", llvm::cl::init(2),
    llvm::cl::desc("1 for Ring32, 2 for Ring64, 3 for Ring128"));
llvm::cl::opt<bool> EngineTrace("engine_trace", llvm::cl::init(false),
                                llvm::cl::desc("Enable trace info"));

std::shared_ptr<yacl::link::Context> MakeLink(const std::string& parties,size_t rank,const GlobalConfig& global_config,std::vector<std::string>& members) {
  yacl::link::ContextDesc lctx_desc;
  std::vector<std::string> hosts = absl::StrSplit(parties, ',');
  for (size_t index = 0; index < hosts.size(); index++) {
    const auto id = fmt::format("party{}", index);
    lctx_desc.parties.push_back({id, hosts[index]});
  }
  lctx_desc.brpc_channel_protocol = "h2:grpc";
  lctx_desc.http_max_payload_size = 64*1024;
  lctx_desc.http_timeout_ms = 60*1000;
  lctx_desc.connect_retry_interval_ms = 500;
  std::shared_ptr<yacl::link::Context> lctx;
  switch (global_config.proxy_config_.proxy_mode_ )
  {
    case 0:
      /* code */
      lctx = yacl::link::FactoryBrpc().CreateContext(lctx_desc, rank);
      break;
    case 1:
      lctx = yacl::link::FactoryBrpcGateway().CreateContextGateway(lctx_desc, rank,members);
      break;
    default:
      SPDLOG_ERROR("proxy mode:{} not supported",global_config.proxy_config_.proxy_mode_ );
      break;
  }
  std::size_t retry_time = 3;
  TryConnectToMesh(lctx,retry_time);
  return lctx;
}

void TryConnectToMesh(std::shared_ptr<yacl::link::Context> lctx,std::size_t retry_time )
{
  try {
    lctx->ConnectToMesh();
  } catch (const std::exception& e) { 
    retry_time--;
    SPDLOG_INFO("TryConnectToMesh:failed less retry_time {} , message {}",retry_time,e.what() );
    if(retry_time > 0)
      TryConnectToMesh(lctx,retry_time);
    else
      throw e;
  }
}

std::unique_ptr<spu::HalContext> MakeHalContext(const std::string& parties,size_t rank,const GlobalConfig& global_config,std::vector<std::string>& members) {
  auto lctx = MakeLink(parties,rank,global_config,members);

  spu::RuntimeConfig config;
  config.set_protocol(static_cast<spu::ProtocolKind>(ProtocolKind.getValue()));
  config.set_field(static_cast<spu::FieldType>(Field.getValue()));

  config.set_enable_action_trace(EngineTrace.getValue());
  config.set_enable_type_checker(EngineTrace.getValue());
  config.set_enable_action_trace(EngineTrace.getValue());
  return std::make_unique<spu::HalContext>(config, lctx);
}


std::string CreateParties(bool testLocal,const GlobalConfig& global_config,uint32_t self_rank,std::vector<std::string>& ips)
{
    std::string parties;
    if(testLocal)
    {
        parties = std::string("127.0.0.1:60021");
        for(std::size_t i=1;i<ips.size();++i)
        {
            parties += std::string(",127.0.0.1:") + std::to_string(60021+i);
        }
        return parties;
    }
    std::string self_port = std::to_string(global_config.grpc_config_.self_port_);
    std::string other_port = std::to_string(global_config.grpc_config_.other_port_);
    std::string gateway_port = std::to_string(global_config.proxy_config_.gateway_config_.port_);
    std::string& self_ip = ips[self_rank];

    switch(global_config.proxy_config_.proxy_mode_ )
    {
        //0 == not proxy
        case 0:
            for(std::size_t i=0;i<ips.size();++i)
            {
                if(i == self_rank){
                    parties += "0.0.0.0:"+self_port + ",";
                } else {
                    parties += ips[i]+":"+other_port+",";
                }
            }
            parties.pop_back();
            break;
        //1 == gate proxy
        // all data send to local gateway server,gateway server will accord member id send to diffierent server
        case 1:
            for(std::size_t i=0;i<ips.size();++i)
            {
                if(i == self_rank){
                    parties += "0.0.0.0:" + self_port+",";
                } else {
                    parties += self_ip+":" + gateway_port+",";
                }
            }
            parties.pop_back();
            break;

        default:
            break;
    }
    return parties;
}

}
}