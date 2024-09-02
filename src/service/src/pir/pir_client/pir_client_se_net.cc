// Copyright 2019 Ant Group Co., Ltd
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
#include <chrono>
#include "pir_client_se_net.h"
#include "src/pir/pir_utils.h"
#include "src/pir/pir_fse_prama.h"
#include "src/rpc/rpc_transport/rpc_session.h"
#include "src/kernel/data_db/db_client.h"

#include "yacl/io/rw/csv_reader.h"
#include "yacl/io/rw/csv_writer.h"
#include "yacl/io/stream/file_io.h"
#include "absl/strings/str_split.h"
#include "apsi_wrapper.h"

namespace mpc{
namespace pir{

PirClientSeNet::~PirClientSeNet()
{
    SPDLOG_INFO("PirClientSeNet::~PirClientSeNet this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
}

void PirClientSeNet::RunServiceImpl()
{
    try {
        SPDLOG_INFO("task_id:{}",parma_.task_id);
        mpc::rpc::GatewayPrama gateway_prama;
        gateway_prama.self_memberid = SelfMemberId();
        gateway_prama.peer_memberid = PeerMemberId();
        gateway_prama.session_id = parma_.task_id;
        gateway_prama.task_id = parma_.task_id;
        auto rpc_session = mpc::rpc::RpcSessionFactory::CreateRpcSession(CreatePeerHost(),gateway_prama);

        rpc_session->SetRecvTimeout(kLinkRecvTimeout);
        // yacl::Buffer data_key_buffer = link_ctx->Recv(link_ctx->NextRank(), fmt::format("recv data_key"));
        // std::string remote_key = std::string{std::string_view(data_key_buffer)};
        // SPDLOG_INFO("get data_key_:{}  task_id:{}",key_,parma_.task_id);
        std::string input_path = GeneratorInputPath(parma_.data_file);

        auto client_sender = [&rpc_session](const std::string& message_key,const std::string& value)
        {
            return rpc_session->Send(message_key,value);
        };
        auto client_recv = [&rpc_session](const std::string& message_key)
        {
            return rpc_session->Recv(message_key);
        };
        DbClient client(input_path, parma_.query_ids, output_path_, part_output_path_, parma_.task_id,parma_.fields, parma_.labels, client_sender, client_recv);
        client.Run();
        server_profile_ = client.ServerProfile();
        // client_profile_ = client.ClientProfile();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("run PirClientSpu failed: {} taskid:{}", e.what(),parma_.task_id);
        status_ = 201;
        error_message_ = e.what();
    }
}

std::string PirClientSeNet::CreateParties()
{
    // return  mpc::utils_fse::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
    return "";
}

void PirClientSeNet::SaveResult(const std::vector<std::string>& fileds,const std::vector<std::string>& labels)
{
    using namespace yacl::io;
    SPDLOG_INFO("output_path_:{}  fileds size:{} intersection size:{} task_id:{}",output_path_,fileds.size(),labels.size(),parma_.task_id);
    std::string file1(output_path_);
    std::unique_ptr<OutputStream> out(new FileOutputStream(file1));

    Schema s;
    s.feature_names.push_back(parma_.fields[0]);
    s.feature_types.push_back(Schema::STRING);
    for(std::size_t i = 0;i<parma_.labels.size();++i)
    {
        s.feature_types.push_back(Schema::STRING);
        s.feature_names.push_back(parma_.labels[i]);
    }
    WriterOptions w_op;
    w_op.file_schema = s;
    CsvWriter writer(w_op, std::move(out));
    writer.Init();
    std::vector<std::string> fields_vec;
    // std::vector<std::string> labels_vec;
    std::vector<std::vector<std::string> > labels_vec(parma_.labels.size());
    ColumnVectorBatch batch;
    for(std::size_t i = 0 ;i<fileds.size();++i)
    {
        // SPDLOG_INFO("save  i:{} value1:{} value1:{}  task_id:{}",i,fileds[i],labels[i],parma_.task_id);
        if(!labels[i].empty())
        {
            // SPDLOG_INFO("save  succeed i:{} value1:{} value1:{}  task_id:{}",i,fileds[i],labels[i],parma_.task_id);
            fields_vec.push_back(fileds[i]);
            std::vector<std::string> substrings = absl::StrSplit(labels[i], ',');
            for(std::size_t j=0;j<substrings.size();++j)
            {
                labels_vec[j].push_back(substrings[j]);
            }
        }
    }   
    batch.AppendCol(fields_vec);
    for(std::size_t i = 0 ;i<labels_vec.size();++i)
        batch.AppendCol(labels_vec[i]);
    writer.Add(batch);
    writer.Close();
    SPDLOG_INFO("66 output_path_:{}  fileds size:{} intersection size:{} task_id:{}",output_path_,fileds.size(),labels.size(),parma_.task_id);
}

}
}