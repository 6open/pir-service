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

#include<unordered_set>

#include "io_wrapper.h"
const std::string ADD = "add";
const std::string MUL = "mul";
const std::string SUBLR = "sublr";
const std::string SUBRL = "subrl";

const std::unordered_set<std::string> g_support_operators = {ADD,MUL,SUBLR,SUBRL};

class ExecutorChecker
{
public:
    ExecutorChecker(const std::unordered_set<std::string>& sp = g_support_operators)
    :support_operators_(sp)
    {}

    bool CheckOp(const std::string& op)
    {
        return support_operators_.count(op) == 1;
    }

    std::pair<bool,std::string> CheckOps(const std::vector<std::string>& ops)
    {
        std::pair<bool,std::string>  ret;
        for(const std::string& op : ops)
        {
            if(!CheckOp(op))
            {
                ret.first = false;
                ret.second = "operator " + op + " not supported";
                return ret;
            }
        }
        ret.first = true;
        return ret;
    }

private:
    const std::unordered_set<std::string>& support_operators_;
};

class OperatorExecutor {
public:
    virtual ~OperatorExecutor() = default;

  //
  // run a kernel in a given region.
    virtual spu::Value runKernelImpl(spu::HalContext *hctx,const std::vector<spu::Value>& input) = 0;

    spu::Value runKernel(spu::HalContext *hctx,const std::vector<spu::Value>& input)
    {
        return runKernelImpl(hctx,input);
    }
private:

};

class AddExecutor : public OperatorExecutor
{
public:
  virtual ~AddExecutor() = default;
  virtual spu::Value runKernelImpl(spu::HalContext *hctx,const std::vector<spu::Value>& input);
};

class MulExecutor : public OperatorExecutor
{
public:
  virtual ~MulExecutor() = default;
  virtual spu::Value runKernelImpl(spu::HalContext *hctx,const std::vector<spu::Value>& input);
};

class SublrExecutor : public OperatorExecutor
{
public:
  virtual ~SublrExecutor() = default;
  virtual spu::Value runKernelImpl(spu::HalContext *hctx,const std::vector<spu::Value>& input);
};

class SubrlExecutor : public OperatorExecutor
{
public:
  virtual ~SubrlExecutor() = default;
  virtual spu::Value runKernelImpl(spu::HalContext *hctx,const std::vector<spu::Value>& input);
};

class ExecutorFactory
{
public:
    static std::unique_ptr<OperatorExecutor> CreateExecutor(const std::string& op)
    {
        if(op == ADD)
            return std::make_unique<AddExecutor>();
        else if(op == MUL)
            return std::make_unique<MulExecutor>();
        else if(op == SUBLR)
            return std::make_unique<SublrExecutor>();
        else if(op == SUBRL)
            return std::make_unique<SubrlExecutor>();
        else
            return nullptr;
    }
};