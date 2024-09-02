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
#include <memory>
#include <mutex>
#include "mega_batch.h"
#include "apsi_wrapper.h"

class DataCacher
{
public:
    DataCacher(const MegaBatch& mb,const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns);
    virtual ~DataCacher() =default;
    void Setup()
    {
        SetupImpl();
    }
    void Remove()
    {
        RemoveImpl();
    }
    bool Loading()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        if(reference_count_ == 0) {
            if(LoadingImpl() == false)
                return false;
        }
        ++reference_count_;
        return true;
    }
    void Release()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        --reference_count_;
        if(reference_count_ == 0 ) {
            ReleaseImpl();
        }
    }
    virtual void SetupImpl() = 0;
    // virtual void RemoveImpl() = 0;
    virtual bool LoadingImpl() = 0;
    virtual void ReleaseImpl() = 0;
    virtual void RemoveImpl() = 0;
protected:
    MegaBatch mega_batch_;
    std::vector<std::string> key_columns_;
    std::vector<std::string> label_columns_;
    std::mutex mutex_;
    std::size_t reference_count_  = 0; 
};

class SeDataCacher : public DataCacher
{
public:
    SeDataCacher(const MegaBatch& mb,const std::vector<std::string>& key_columns,const std::vector<std::string>& label_columns);
    virtual ~SeDataCacher() = default;
    void SetupImpl() override;
    void GetItem(const std::string& input_path, std::vector<std::pair<std::string, std::string>>& vec_item,std::size_t& max_label_size);
    bool LoadingImpl() override;
    void ReleaseImpl() override;
    void RemoveImpl() override;
    apsi::wrapper::APSIServerWrapper* GetApsiServer()
    {
        return server_wrapper_.get();
    }
public:
    std::unique_ptr<apsi::wrapper::APSIServerWrapper> server_wrapper_;
};