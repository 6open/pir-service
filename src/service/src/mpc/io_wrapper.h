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

#include "libspu/device/io.h"
#include "libspu/kernel/context.h"


class IoWrapper
{
public:
    IoWrapper(const std::string& taskId,spu::HalContext *hctx);
    ~IoWrapper() = default;

    void sync()
    {
        cio_.sync();
    }
    void hostSetVar(const std::string &name, spu::PtBufferView bv);
    spu::Value deviceGetVar(const std::string &name,std::size_t rank) const;
    // spu::Value hostGetVar(const std::string &name,std::size_t rank) const;
private:
    std::string fmtkey(const std::string &name) const;
    std::string fmtkey(const std::string &name,std::size_t rank) const;

private:
    std::string task_id_;
    spu::HalContext* hctx_;
    spu::device::ColocatedIo cio_;
};