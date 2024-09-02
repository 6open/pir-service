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

#include "data_cacher.h"
#include <filesystem>

#include "yacl/io/stream/file_io.h"
#include "yacl/io/rw/csv_reader.h"
#include "src/pir/pir_fse_prama.h"
#include "spdlog/spdlog.h"


DataCacher::DataCacher(const MegaBatch& mb,const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns)
:mega_batch_(mb)
,key_columns_(key_columns)
,label_columns_(label_columns)
{

}
SeDataCacher::SeDataCacher(const MegaBatch& mb,const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns)
:DataCacher(mb,key_columns,label_columns)
{

}

void SeDataCacher::SetupImpl()
{
    std::size_t max_label_size = 10;
    std::size_t expect_size = mega_batch_.TotalSize()+100;
    std::vector<std::pair<std::string, std::string>> vec_item;
    SPDLOG_INFO("SeDataCacher::SetupImpl this {} reserve:{}",fmt::ptr(static_cast<void*>(this)),expect_size);
    vec_item.reserve(expect_size);
    // SPDLOG_INFO("SeDataCacher::SetupImpl this {} reserve:{}",fmt::ptr(static_cast<void*>(this)),expect_size);
    std::vector<std::string>& batch_names = mega_batch_.batch_names_;
    for(const std::string& str : batch_names)
    {
        GetItem(str,vec_item,max_label_size);
    }
    apsi::wrapper::APSIServerWrapper server_wrapper;
    SPDLOG_INFO("SeDataCacher::GetItem succeed this {} expect_size:{},real size:{}",fmt::ptr(static_cast<void*>(this)),expect_size,vec_item.size());
    server_wrapper.init_db(kPirFseParma, max_label_size, 16, false);
    // SPDLOG_INFO("SeDataCacher::SetupImpl this {} reserve:{} end2",fmt::ptr(static_cast<void*>(this)),expect_size);
    server_wrapper.add_item(vec_item);
    SPDLOG_INFO("SeDataCacher::add_item succeed this {} expect_size:{},real size:{},max_label_size:{}",fmt::ptr(static_cast<void*>(this)),expect_size,vec_item.size(),max_label_size);
    // SPDLOG_INFO("SeDataCacher::SetupImpl this {} reserve:{} end3",fmt::ptr(static_cast<void*>(this)),expect_size);
    if (std::filesystem::exists(mega_batch_.DataCacheName())) {
        SPDLOG_INFO("SeDataCacher::SetupImpl this {} rm:{}",fmt::ptr(static_cast<void*>(this)),mega_batch_.DataCacheName());
        std::filesystem::remove(mega_batch_.DataCacheName());
    }
    server_wrapper.save_db(mega_batch_.DataCacheName());
    SPDLOG_INFO("SeDataCacher::save_db succeed this {} expect_size:{},real size:{},mega_batch_.DataCacheName:{}",fmt::ptr(static_cast<void*>(this)),expect_size,vec_item.size(),mega_batch_.DataCacheName());
}

void SeDataCacher::GetItem(const std::string& input_path, std::vector<std::pair<std::string, std::string>>& vec_item,std::size_t& max_label_size)
{
    SPDLOG_INFO("SeDataCacher::GetItem this {} GetItem:{}",fmt::ptr(static_cast<void*>(this)),input_path);
    using namespace yacl::io;
    Schema s;
    for(const std::string& str : key_columns_)
    {
        s.feature_types.push_back(Schema::STRING);
        s.feature_names.push_back(str);
    }
    for(const std::string& str : label_columns_)
    {
        s.feature_types.push_back(Schema::STRING);
        s.feature_names.push_back(str);
    }

    ReaderOptions r_ops;
    //r_ops.column_reader = true;
    r_ops.file_schema = s;
    r_ops.batch_size = 1000;
    r_ops.use_header_order = true;
    r_ops.column_reader = false;

    std::unique_ptr<InputStream> in(new FileInputStream(input_path));
    CsvReader reader(r_ops, std::move(in));
    ColumnVectorBatch batch;
    reader.Init();
    // SPDLOG_INFO(" begin load data to db id:{} label size:{} task_id:{}",parma_.fields[0],parma_.labels.size(),parma_.task_id);
    // std::size_t batch_count = 0;
   
    // std::size_t max_label_size = 10;
    while(reader.Next(r_ops.batch_size,&batch))
    {
        // SPDLOG_INFO(" get batch:{} row:{},col:{}",batch_count,batch.Shape().rows,batch.Shape().cols);
        for(std::size_t i=0;i<batch.Shape().rows;++i)
        {
            std::string str;
            for(std::size_t j=1;j<batch.Shape().cols;++j)
            {
                str += batch.At<std::string>(i, j)+",";
            }
            str.pop_back();
            if(str.size() > max_label_size) {
                SPDLOG_INFO(" update max_label_size:{} str size:{} str:{}",max_label_size,str.size(),str);
                max_label_size = str.size();
            }
            // max_label_size = std::max(max_label_size,str.size());
            // SPDLOG_INFO(" add_item  i:{} id:{},label:{}",i,batch.At<std::string>(i, 0),str);
            vec_item.emplace_back(std::make_pair(batch.At<std::string>(i, 0),str));
            //server_wrapper_.add_item(batch.At<std::string>(i, 0),batch.At<std::string>(i, 1));
        }
        // SPDLOG_INFO(" get batch:{} end row:{},col:{}",batch_count++,batch.Shape().rows,batch.Shape().cols);
        // SPDLOG_INFO(" get batch first:{},second:{}",batch.At<std::string>(0, 0),batch.At<std::string>(0, 1));
    }
    // SPDLOG_INFO(" end  get data to db max_label_size:{} task_id:{}",max_label_size,parma_.task_id);
    if (std::filesystem::exists(input_path)) {
        SPDLOG_INFO("SeDataCacher::GetItem this {} rm:{}",fmt::ptr(static_cast<void*>(this)),mega_batch_.DataCacheName());
        std::filesystem::remove(input_path);
    }
}

bool SeDataCacher::LoadingImpl()
{
    if (!std::filesystem::exists(mega_batch_.DataCacheName())) {
        SPDLOG_ERROR("Cache file not exist : {}", mega_batch_.DataCacheName());
        return false;
    }
    SPDLOG_INFO("SeDataCacher::LoadingImpl this {} name:{}",fmt::ptr(static_cast<void*>(this)),mega_batch_.DataCacheName());
    try {
        if(server_wrapper_ == nullptr)
            server_wrapper_ = std::make_unique<apsi::wrapper::APSIServerWrapper>();
        server_wrapper_->load_db(mega_batch_.DataCacheName());

        SPDLOG_INFO("SeDataCacher::LoadingImpl this{} name:{} succeed",fmt::ptr(static_cast<void*>(this)),mega_batch_.DataCacheName());
    } catch (std::exception& e)
    {
        SPDLOG_INFO("SeDataCacher::LoadingImpl this {} error:{}",fmt::ptr(static_cast<void*>(this)),e.what());
        ReleaseImpl();
        return false;
    }
    // server_wrapper_ = nullptr;
    return true;
}

void SeDataCacher::ReleaseImpl()
{
    SPDLOG_INFO("SeDataCacher::ReleaseImpl this {}",fmt::ptr(static_cast<void*>(this)));
    try {
        if(server_wrapper_ != nullptr)
            server_wrapper_ = nullptr;
    } catch (std::exception& e)
    {
        SPDLOG_INFO("SeDataCacher::ReleaseImpl this {} error:{}",fmt::ptr(static_cast<void*>(this)),e.what());
    }
}

void SeDataCacher::RemoveImpl()
{
    // if(std::remove)
}