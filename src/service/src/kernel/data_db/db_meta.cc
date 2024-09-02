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

#include "db_meta.h"
#include <nlohmann/json.hpp>
#include "src/pir/pir_fse_prama.h"
#include <bitset>
#include <filesystem>
#include "batch_writer.h"
#include "libspu/psi/utils/batch_provider.h"
#include "spdlog/spdlog.h"
#include "src/config/config.h"
#include <mlflow_api.h>

namespace fs = std::filesystem;

const char* MLFLOW_URI = std::getenv("MLFLOW_TRACKING_URI");

std::size_t CsvFileDataCount(const std::string &file_path,
                        const std::vector<std::string> &ids) {
    size_t data_count = 0;

    std::shared_ptr<spu::psi::IBatchProvider> batch_provider =
        std::make_shared<spu::psi::CsvBatchProvider>(file_path, ids);

    while (true) {
        auto batch = batch_provider->ReadNextBatch(4096);
        if (batch.empty()) {
        break;
        }
        data_count += batch.size();
    }

    return data_count;
}

DbMeta::DbMeta(const std::string& key,const std::string& meta_dir,const std::string& data_path,
    const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns,std::size_t per_batch_size)
:meta_info_(key,GeneratorSelfMetaDir(key,meta_dir),key_columns,label_columns)
,data_path_(data_path)
,per_batch_size_(per_batch_size)
{
    SPDLOG_INFO("DbMeta::DbMeta key:{} meta_dir_:{} data_path:{} per_batch_size_:{}", meta_info_.key_,meta_info_.meta_path_,data_path,per_batch_size_);
    if (!fs::exists(meta_info_.meta_path_)) {
        fs::create_directory(meta_info_.meta_path_);
    }
}

DbMeta::DbMeta(const DbMetaInfo& metainfo)
:meta_info_(metainfo)
{
    SetupSucceed();
    sender_cacher_ = std::make_unique<SenderCacher>(meta_info_);
}

std::string DbMeta::GeneratorSelfMetaDir(const std::string& key,const std::string& meta_dir)
{
    return meta_dir+"/meta_"+key;
}

std::string DbMeta::GeneratorBatchName(const std::string& file_path,std::size_t num)
{
    return file_path+"/tempor"+std::to_string(num)+".bh";
}

std::size_t DbMeta::GetBatchCount(const std::string& str,std::size_t batch_size)
{
    // return  (*reinterpret_cast<const uint64_t*>(str.c_str())) % batch_size;
    std::hash<std::string> hasher;
    uint64_t hash_value = hasher(str);
    return hash_value % batch_size;
}

void DbMeta::GeneratorBatch()
{
    auto& key_columns_ = meta_info_.key_columns_;
    auto& label_columns_ = meta_info_.label_columns_; 
    std::size_t server_data_count = CsvFileDataCount(data_path_, key_columns_);
    std::size_t batch_size = (server_data_count+kPerBatchSize)/kPerBatchSize;
    SPDLOG_INFO("DbMeta::GeneratorBatch this {} key:{} server_data_count:{} batch_size:{}",fmt::ptr(static_cast<void*>(this)), meta_info_.key_,server_data_count,batch_size);
    std::vector<std::unique_ptr<BatchWriter>> batch_writers;
    std::vector<std::string> batch_names;

    for(std::size_t i=0;i<batch_size;++i)
    {
        batch_names.push_back(GeneratorBatchName(meta_info_.meta_path_,i));
        std::unique_ptr<BatchWriter> bw = std::make_unique<BatchWriter>(batch_names.back(),key_columns_,label_columns_);
        batch_writers.push_back(std::move(bw));
    }
    std::shared_ptr<spu::psi::IBatchProvider> batch_provider = std::make_shared<spu::psi::CsvBatchProvider>(data_path_,key_columns_, label_columns_);
    while(true)
    {
        std::vector<std::string> filed_result;
        std::vector<std::string> label_result;
        std::tie(filed_result,label_result) =  batch_provider->ReadNextBatchWithLabel(4096);

        if(filed_result.empty())
            break;
        for(std::size_t i=0;i<filed_result.size();++i)
        {
            std::size_t batch_num = GetBatchCount(filed_result[i],batch_size);
            batch_writers[batch_num]->AddItem(filed_result[i],label_result[i]);
        }
    }
    //flush all data and close file
    std::vector<std::size_t> num_count;
    for(std::size_t i=0;i<batch_writers.size();++i)
    {
        batch_writers[i]->Release();
        num_count.push_back(batch_writers[i]->TotalCount());
    }

    // SPDLOG_INFO("DbMeta::GeneratorBatch this {} key:{} batch_names:{} num_count:{}",fmt::ptr(static_cast<void*>(this)), key_,batch_names,num_count);
    meta_info_.SetBatch(batch_names, num_count);
    // meta_info_ = DbMetaInfo(key_,meta_dir_,batch_names,num_count,key_columns_,label_columns_);
    meta_info_.Serialize(DbMetaInfo::CreateDbMetaInfoPath(meta_info_.meta_path_)); 
    sender_cacher_ = std::make_unique<SenderCacher>(meta_info_);
    sender_cacher_->Setup();
}

void DbMeta::Setup()
{
    if(IsSetup())
    {
        SPDLOG_INFO("DbMeta::Setup this {} has been setuped",fmt::ptr(static_cast<void*>(this)));
        return;
    }
    try{
        GeneratorBatch();
    }
    catch(const std::exception& e)
    {
        Remove();
        throw ;
    }
    
    SetupSucceed();
}

void DbMeta::Remove()
{
    SPDLOG_INFO("DbMeta::Remove this {} removed all key_:{}",fmt::ptr(static_cast<void*>(this)),meta_info_.key_);
    fs::remove_all(meta_info_.meta_path_);
}

std::unique_ptr<DbMeta> DbMeta::LoadCahce(const std::string& dir_path)
{
    std::filesystem::path path(dir_path);
    std::string file_name = path.filename().string();
    if(file_name == "pir_result")
        return nullptr;

    // SPDLOG_INFO("DbMeta::LoadCahce  dir:{}",dir_path);
    if(!std::filesystem::is_directory(std::filesystem::status(dir_path)))
    {
        return nullptr;
    }
    std::vector<std::string> fileNames;
    for (const auto& entry : std::filesystem::directory_iterator(dir_path)) {
        if (!std::filesystem::is_directory(entry.status())) {
            // 如果是子目录，则递归遍历
            fileNames.push_back(entry.path().filename().string());
        }
    }
    std::string key;
    std::string meta_name;
    for(const std::string& str : fileNames)
    {
        if(str == "metainfo")
        {
            // key = str.substr(0,pos-1);
            meta_name = dir_path+"/"+str;
            break;
        }
    }
    if(meta_name.empty())
    {
        SPDLOG_INFO("DbMeta::LoadCahce  cannot find metainfo file dir:{}",dir_path);
        return nullptr;
    }
    try{
        DbMetaInfo meta_info;
        meta_info.Deserialize(meta_name);
        std::unique_ptr<DbMeta> dbmeta = std::make_unique<DbMeta>(meta_info);
        if(dbmeta->sender_cacher_->CheckLoading() == false)
        {
            return nullptr;
        }
        return dbmeta;
    }
    catch(const std::exception& e)
    {
        return nullptr;
    }
    return nullptr;
}

void DbMeta::ClearCacheData(const std::string& key)
{
    std::string data_name = "meta_" + key;
    std::string delete_directory = GetGlobalConfig().pir_config_.data_meta_path + "/" + data_name;
    fs::remove_all(delete_directory);
    std::cout << "Deleted directory: " << data_name << std::endl;
}

DbMeta* DbMetaManager::GetDbMeta(const std::string& key,const std::string& data_path,
    const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = db_metas_.find(key);
    if(iter != db_metas_.end())
    {
        SPDLOG_INFO("key:{} exit ", key);
        return iter->second.get();
    }
    else
    {
        std::unique_ptr<DbMeta> db_meta = std::make_unique<DbMeta>(key,meta_dir_,data_path,key_columns,label_columns);
        auto ret = db_metas_.insert(std::make_pair(key,std::move(db_meta)));
        return ret.first->second.get();
    }
}

DbMeta* DbMetaManager::GetDbMeta(const std::string& key)
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = db_metas_.find(key);
    if(iter != db_metas_.end())
        return iter->second.get();
    else 
        return nullptr;
}

void DbMetaManager::LoadingCache()
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& entry : std::filesystem::directory_iterator(meta_dir_)) {
        if (std::filesystem::is_directory(entry.status())) {
            std::string child_dir = meta_dir_+"/"+entry.path().filename().string();
            std::unique_ptr<DbMeta> dbmeta = DbMeta::LoadCahce(child_dir);
            if(dbmeta != nullptr)
            {
                std::string key = dbmeta->Key();
                SPDLOG_INFO("DbMeta::LoadingCache   file dir:{} key:{}",child_dir,key);
                db_metas_.insert({key,std::move(dbmeta)});
            }
        }
    }
}