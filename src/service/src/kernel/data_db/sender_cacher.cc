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

#include "sender_cacher.h"
#include <sys/sysinfo.h>
#include <filesystem>
#include "spdlog/spdlog.h"


void SenderCacher::Setup()
{
    std::vector<MegaBatch>& mega_batchs = meta_info_.mega_batchs_;
    data_caches_.reserve(mega_batchs.size());
    // for(std::size_t i)
    for(const MegaBatch& mb : mega_batchs)
    {
        data_caches_.push_back(std::make_unique<SeDataCacher>(mb,meta_info_.key_columns_, meta_info_.label_columns_));
        data_caches_.back()->Setup();
    }
}

double SenderCacher::GetSysAviMemory()
{
    struct sysinfo info;
    if (sysinfo(&info) != 0) {
        SPDLOG_INFO( "Failed to retrieve system information." );
        return 0;
    }
    double totalMemory = static_cast<double>(info.totalram * info.mem_unit) / (1024 * 1024 * 1024);
    double freeMemory = static_cast<double>(info.freeram * info.mem_unit) / (1024 * 1024 * 1024);
    SPDLOG_INFO("SenderCacher::GetSysAviMemory totalMemory:{} freeMemory:{} batch_count:{}",totalMemory,freeMemory);
    return static_cast<double>(freeMemory) / (1024 * 1024 * 1024);
}

bool SenderCacher::CheckLoading()
{
    std::vector<MegaBatch>& mega_batchs = meta_info_.mega_batchs_;
    for(const MegaBatch& mb : mega_batchs)
    {
        data_caches_.push_back(std::make_unique<SeDataCacher>(mb,meta_info_.key_columns_, meta_info_.label_columns_));
    }
    return true;
}