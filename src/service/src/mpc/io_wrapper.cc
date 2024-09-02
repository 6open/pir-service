// Copyright 2022 Ant Group Co., Ltd.
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

#include "io_wrapper.h"

IoWrapper::IoWrapper(const std::string& taskId,spu::HalContext *hctx)
    :task_id_(taskId)
    ,hctx_(hctx)
    ,cio_(hctx)
{

}

void IoWrapper::hostSetVar(const std::string &name, spu::PtBufferView bv)
{
    SPDLOG_INFO("IoWrapper::hostSetVar self_rank:{} {}",hctx_->lctx()->Rank(),fmtkey(name));
    cio_.hostSetVar(fmtkey(name),bv);
}

spu::Value IoWrapper::deviceGetVar(const std::string &name,std::size_t rank) const
{
    SPDLOG_INFO("IoWrapper::deviceGetVar self_rank:{} {}",hctx_->lctx()->Rank(),fmtkey(name,rank));
    return cio_.deviceGetVar(fmtkey(name,rank));
}

// spu::Value IoWrapper::hostGetVar(const std::string &name,std::size_t rank) const
// {
//     SPDLOG_INFO("IoWrapper::hostGetVar self_rank:{} {}",hctx_->lctx()->Rank(),fmtkey(name,rank));
//     return cio_.hostGetVar(fmtkey(name,rank));
// }

std::string IoWrapper::fmtkey(const std::string &name) const
{
    return fmtkey(name,hctx_->lctx()->Rank());
}

std::string IoWrapper::fmtkey(const std::string &name,std::size_t rank) const
{
    return fmt::format("{}_{}_{}", task_id_ ,name,rank);
}