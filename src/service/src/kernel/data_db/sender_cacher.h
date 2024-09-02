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
#include "db_meta_info.h"
#include "data_cacher.h"
class SenderCacher
{
public:
    SenderCacher(const DbMetaInfo& metainfo)
    :meta_info_(metainfo)
    {}
    SenderCacher(const std::string& filename)
    {
        meta_info_.Deserialize(filename);
    }
    void Setup();
    DataCacher* GetDataCacher(std::size_t i)
    {
        return data_caches_[i].get();
    }
    double GetSysAviMemory();

    bool CheckLoading();
public:
    DbMetaInfo meta_info_;
    std::vector<std::unique_ptr<DataCacher>> data_caches_;
};