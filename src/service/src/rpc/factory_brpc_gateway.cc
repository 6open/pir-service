
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

#include "factory_brpc_gateway.h"
#include "receiver_brpc_gateway.h"
#include "channel_brpc_gateway.h"

namespace yacl::link {

std::shared_ptr<Context> FactoryBrpcGateway::CreateContextGateway(const ContextDesc& desc,
                                                    size_t self_rank,const std::vector<std::string>& member_ids) {
  const size_t world_size = desc.parties.size();
  if (self_rank >= world_size) {
    YACL_THROW_LOGIC_ERROR("invalid self rank={}, world_size={}", self_rank,
                           world_size);
  }

  auto msg_loop = std::make_unique<ReceiverLoopBrpcGateway>();
  std::vector<std::shared_ptr<IChannel>> channels(world_size);
  const std::string& self_member_id = member_ids[self_rank];
  for (size_t rank = 0; rank < world_size; rank++) {
    if (rank == self_rank) {
      continue;
    }
    const std::string& peer_member_id = member_ids[rank];

    ChannelBrpcGateWay::Options opts;
    opts.http_timeout_ms = desc.http_timeout_ms;
    opts.http_max_payload_size = desc.http_max_payload_size;
    opts.channel_protocol = desc.brpc_channel_protocol;
    opts.channel_connection_type = desc.brpc_channel_connection_type;

    opts.self_member_id = self_member_id;
    opts.peer_member_id = peer_member_id;
    opts.session_id = self_member_id+"_"+peer_member_id;

    auto channel = std::make_shared<ChannelBrpcGateWay>(self_rank, rank,
                                                 desc.recv_timeout_ms, opts);
    channel->SetPeerHost(desc.parties[rank].host);
    channel->SetThrottleWindowSize(desc.throttle_window_size);

    msg_loop->AddListener(rank, channel);
    channels[rank] = std::move(channel);
  }

  // start receiver loop.
  const auto self_host = desc.parties[self_rank].host;
  msg_loop->Start(self_host);

  return std::make_shared<Context>(desc, self_rank, std::move(channels),
                                   std::move(msg_loop));
}

}  // namespace yacl::link
