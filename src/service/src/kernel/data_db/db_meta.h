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
#pragma once

#include <string>
#include <mutex>
#include <map>
#include <filesystem>
#include "sender_cacher.h"
// #include "spulib/psi/utils/"

// const std::size_t kPerBatchSize = 200000;
const std::size_t kPerBatchSize = 1000000;
// const std::size_t kPerBatchSize = 100000;
class DbMeta
{
public:
    DbMeta(const std::string& key,const std::string& meta_dir,const std::string& data_path,
    const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns,std::size_t per_batch_size = kPerBatchSize);
    DbMeta(const DbMetaInfo& metainfo);

    void GeneratorBatch();
    void Setup();
    bool IsSetup() const {return is_setuped_;}
    void SetupSucceed() {is_setuped_ = true;}
    void Remove();
    void PushDataToFlow();
    void GetDataFromMlflowByKey(const std::string& key);
    void ClearCacheData(const std::string& key);

    static std::string GeneratorSelfMetaDir(const std::string& key,const std::string& meta_dir);
    static std::string GeneratorBatchName(const std::string& file_path,std::size_t num);
    static std::size_t GetBatchCount(const std::string& str,std::size_t batch_size);

    const DbMetaInfo& MetaInfo() const {return meta_info_;}
    DataCacher* GetDataCacher(std::size_t i)
    {
        // return sender_cacher_->GetDataCacher(MetaInfo().GetMergedBatch(i));
        return sender_cacher_->GetDataCacher(i);
    }
    static std::unique_ptr<DbMeta> LoadCahce(const std::string& dir_path);
    std::string Key() {return meta_info_.key_;}
public:
    DbMetaInfo meta_info_;
    std::string data_path_;
    std::size_t per_batch_size_;
    std::unique_ptr<SenderCacher> sender_cacher_ = nullptr;
    std::mutex setup_mutex_;
    bool is_setuped_ = false;
};

class DbMetaManager
{
public:
    DbMetaManager() = default;

    //will create meta if not exit
    DbMeta* GetDbMeta(const std::string& key,const std::string& data_path,
    const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns);
    //will not create if not exit
    DbMeta* GetDbMeta(const std::string& key);
    void SetMetaDir(const std::string& meta_dir)
    {
        meta_dir_ = meta_dir;
        CreateMetaDir();
    }
    void CreateMetaDir()
    {
        if (!std::filesystem::exists(meta_dir_)) {
            std::filesystem::create_directory(meta_dir_);
        }
    }

    void LoadingCache();
    void GetAllDataFromMlflow();
    void ClearCacheFile(const std::string& directory, const std::string& filename_to_keep);
    static DbMetaManager& Instance()
    {
        static DbMetaManager instance;
        return instance;
    }
    std::map<std::string,std::unique_ptr<DbMeta>>& GetMetas()
    {
        return db_metas_;
    }
private:
    std::string meta_dir_;
    std::mutex  mutex_;
    std::map<std::string,std::unique_ptr<DbMeta>> db_metas_;
};

class FileDeleter {
public:
    FileDeleter(){}

    void setFileName(const std::string filePath){
        filePath_ = filePath;
    }
    FileDeleter(const FileDeleter&) = delete;
    FileDeleter& operator=(const FileDeleter&) = delete;
    FileDeleter(FileDeleter&& other) noexcept : filePath_(std::move(other.filePath_)) {
        other.filePath_.clear();
    }
    FileDeleter& operator=(FileDeleter&& other) noexcept {
        if (this != &other) {
            filePath_ = std::move(other.filePath_);
            other.filePath_.clear();
        }
        return *this;
    }
    ~FileDeleter() {
        if (!filePath_.empty()) {
            if (std::remove(filePath_.c_str()) != 0) {
                std::cout << "Error deleting file : " << filePath_ << std::endl;
            }
        }
    }

private:
    std::string filePath_; 
};