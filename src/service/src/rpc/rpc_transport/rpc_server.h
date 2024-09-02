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
#include <string>
#include <functional>
#include "rpc_receiver.h"

#include "brpc/server.h"
#include "bthread/condition_variable.h"
namespace mpc {
namespace rpc {

class RpcServer
{
public:
    RpcServer();
    ~RpcServer();

    static RpcServer& GetInstance() {
        static RpcServer instance;
        return instance;
    }
    void Stop();

    // start the receiver loop.
    // message received from peers will be listened and dispatched by this loop.
    //
    // host: the desired listen addr:port pair.
    // returns: the actual listening addr:port pair.
    //
    // Note: brpc support "ip:0" listen mode, in which brpc will try to find a
    // free port to listen.
    std::string Start(const std::string& host);

    std::shared_ptr<RpcReceiver> AddReceiver(const std::string& key);
    std::shared_ptr<RpcReceiver> GetReceiver(const std::string& key);
    void RemoveReceiver(const std::string& key);

    void OnMessage(const std::string& key,const std::string& message,std::map<std::string, std::string>& msg_config,std::string& errorInfo);
private:
    void StopImpl();

protected:
    brpc::Server server_;
    std::map<std::string,std::string> msg_;
    std::map<std::string,std::shared_ptr<RpcReceiver>> receivers_;
    bthread::Mutex msg_mutex_;

    bthread::ConditionVariable msg_db_cond_;
    bthread::Mutex receiver_mutex_;


};

} // namespace rpc
} // namespace mpc