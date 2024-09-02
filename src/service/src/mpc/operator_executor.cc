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

#include "operator_executor.h"

#include "libspu/kernel/hal/hal.h"


spu::Value AddExecutor::runKernelImpl(spu::HalContext* hctx,const std::vector<spu::Value>& input)
{
    //io_wrapper->hostSetVar("add",bv);
    //io_wrapper->sync();
    //std::size_t total_node_size = config.total_rank_;
    YACL_ENFORCE(input.size() > 1,"add need at least two input");
    spu::Value result = spu::kernel::hal::add(hctx,input[0],input[1]);
    for(std::size_t i=2;i<input.size();++i)
        result = spu::kernel::hal::add(hctx,result,input[i]);
    return result;
}

spu::Value MulExecutor::runKernelImpl(spu::HalContext* hctx,const std::vector<spu::Value>& input)
{
    YACL_ENFORCE(input.size() > 1,"mul need at least two input");
    spu::Value result = spu::kernel::hal::mul(hctx,input[0],input[1]);
    for(std::size_t i=2;i<input.size();++i)
        result = spu::kernel::hal::mul(hctx,result,input[i]);
    return result;
}

spu::Value SublrExecutor::runKernelImpl(spu::HalContext* hctx,const std::vector<spu::Value>& input)
{
    YACL_ENFORCE(input.size() > 1,"sublr need at least two input");
    spu::Value result = spu::kernel::hal::sub(hctx,input[0],input[1]);
    return result;
}

spu::Value SubrlExecutor::runKernelImpl(spu::HalContext* hctx,const std::vector<spu::Value>& input)
{
    YACL_ENFORCE(input.size() > 1,"subrl need at least two input");
    spu::Value result = spu::kernel::hal::sub(hctx,input[1],input[0]);
    return result;
}
