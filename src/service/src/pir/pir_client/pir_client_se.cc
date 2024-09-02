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
#include "pir_client_se.h"
#include "src/pir/pir_utils.h"
#include "src/pir/pir_fse_prama.h"

#include "yacl/io/rw/csv_reader.h"
#include "yacl/io/rw/csv_writer.h"
#include "yacl/io/stream/file_io.h"
#include "absl/strings/str_split.h"
#include "apsi_wrapper.h"

namespace mpc{
namespace pir{

PirClientSe::~PirClientSe()
{
    SPDLOG_INFO("PirClientSe::~PirClientSe this {} task_id:{}",fmt::ptr(static_cast<void*>(this)),parma_.task_id);
}

void PirClientSe::RunServiceImpl()
{
    try {
        SPDLOG_INFO("task_id:{}",parma_.task_id);
        auto hctx = MakeHalContext();
        auto link_ctx = hctx->lctx();

        link_ctx->SetRecvTimeout(kLinkRecvTimeout);
        // yacl::Buffer data_key_buffer = link_ctx->Recv(link_ctx->NextRank(), fmt::format("recv data_key"));
        // std::string remote_key = std::string{std::string_view(data_key_buffer)};
        // SPDLOG_INFO("get data_key_:{}  task_id:{}",key_,parma_.task_id);
        using namespace yacl::io;
        std::string input_path = GeneratorInputPath(parma_.data);
        SPDLOG_INFO("input_path size:{}  task_id:{}",input_path,parma_.task_id);
        std::unique_ptr<InputStream> in(new FileInputStream(input_path));
        Schema s;
        s.feature_types = {Schema::STRING};
        s.feature_names.push_back(parma_.fields[0]);
        ReaderOptions r_ops;
        //r_ops.column_reader = true;
        r_ops.file_schema = s;
        r_ops.batch_size = 1;
        r_ops.use_header_order = true;
        r_ops.column_reader = true;
        CsvReader reader(r_ops, std::move(in));
        ColumnVectorBatch batch;
        reader.Init();
        reader.Next(r_ops.batch_size,&batch);
        const std::vector<std::string>& query_data = batch.Col<std::string>(0);
        
        //apsi每次查询的数据量 * 1.2 不得大于参数中的table_size
        std::vector<std::vector<std::string>> query_part;
        nlohmann::json json_data = nlohmann::json::parse(kPirFseParma);
        int table_size = json_data["table_params"]["table_size"];
        size_t batch_size = table_size * 10 / 12;
        size_t batch_count = query_data.size()/batch_size;
        if(query_data.size()%batch_size) batch_count++;

        SPDLOG_INFO("query_data size:{}, table_size:{}, batch size:{}, batch_count:{}, task_id:{}",
            query_data.size(), table_size, batch_size, batch_count, parma_.task_id);

        for (size_t i = 0; i < query_data.size(); i += batch_size) {
            std::vector<std::string> partition;
            size_t endIndex = std::min(i + batch_size, query_data.size());
            std::copy_n(query_data.begin() + i, endIndex - i, std::back_inserter(partition));
            query_part.push_back(partition);
        }

        std::string parma(kPirFseParma);
        vector<string> intersections;
        vector<string> mergeData;

        yacl::Buffer batch_count_buffer(&batch_count, sizeof(batch_count));  // 将整数放入缓冲区
        link_ctx->SendAsync(link_ctx->NextRank(), batch_count_buffer, "batch_count_buffer");
        for(size_t i = 0; i<batch_count; i++){
            apsi::wrapper::APSIClientWrapper client_wrapper(parma);
            std::string oprf_request = client_wrapper.oprf_request(query_part[i]);
            yacl::Buffer oprf_buffer(oprf_request.data(),oprf_request.size());
            link_ctx->SendAsync(link_ctx->NextRank(), oprf_buffer,
                        fmt::format("send oprf_request"));
            yacl::Buffer oprf_reponse_buffer = link_ctx->Recv(link_ctx->NextRank(), fmt::format("recv oprf_response"));
            std::string oprf_response{std::string_view(oprf_reponse_buffer)};
            std::string query_string = client_wrapper.build_query(oprf_response);
            yacl::Buffer query_string_buffer(query_string.data(),query_string.size());
            link_ctx->SendAsync(link_ctx->NextRank(), query_string_buffer,fmt::format("send query_string"));
            yacl::Buffer response_string_buffer = link_ctx->Recv(link_ctx->NextRank(), fmt::format("recv response_string"));
            std::string response_string{std::string_view(response_string_buffer)};
            vector<string> intersection = client_wrapper.extract_labeled_result(response_string);
            SPDLOG_INFO("extract_labeled_result  fileds size:{} intersection size:{} task_id:{}",query_part[i].size(),intersection.size(),parma_.task_id);
            mergeData.insert(mergeData.end(), query_part[i].begin(), query_part[i].end());
            intersections.insert(intersections.end(), intersection.begin(), intersection.end());
        }
        SaveResult(mergeData,intersections);

    } catch (const std::exception& e) {
        SPDLOG_ERROR("run PirClientSpu failed: {} taskid:{}", e.what(),parma_.task_id);
        status_ = 201;
        error_message_ = e.what();
    }
}

std::string PirClientSe::CreateParties()
{
    return  mpc::utils_fse::CreateParties(parma_.testLocal,global_config_,parma_.rank,parma_.ips);
}

void PirClientSe::SaveResult(const std::vector<std::string>& fileds,const std::vector<std::string>& labels)
{
    using namespace yacl::io;
    SPDLOG_INFO("output_path_:{}  fileds size:{} intersection size:{} task_id:{}",output_path_,fileds.size(),labels.size(),parma_.task_id);
    std::string file1(output_path_);
    std::unique_ptr<OutputStream> out(new FileOutputStream(file1));
    if(!out->good()){
        throw std::ios_base::failure("Open result file failed : {}" + output_path_);
    }

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

std::unique_ptr<spu::HalContext> PirClientSe::MakeHalContext()
{
    return mpc::utils_fse::MakeHalContext(parties_,parma_.rank,global_config_,parma_.members);
}
}
}