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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <utility>

#include "brpc/channel.h"
#include "brpc/server.h"
#include "bthread/bthread.h"
#include "bthread/condition_variable.h"

#include "yacl/link/transport/channel.h"

namespace yacl::link {

class ReceiverLoopBrpcGateway final : public ReceiverLoopBase {
 public:
  ~ReceiverLoopBrpcGateway() override;

  void Stop() override;

  // start the receiver loop.
  // message received from peers will be listened and dispatched by this loop.
  //
  // host: the desired listen addr:port pair.
  // returns: the actual listening addr:port pair.
  //
  // Note: brpc support "ip:0" listen mode, in which brpc will try to find a
  // free port to listen.
  std::string Start(const std::string& host);

 protected:
  brpc::Server server_;

 private:
  void StopImpl();
};

}  // namespace yacl::link
