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

#include "llvm/Support/CommandLine.h"

#include "mock_spu_context.h"

#include "src/config/config.h"
namespace mpc{
namespace utils_fse{

extern llvm::cl::opt<std::string> Parties;
extern llvm::cl::opt<uint32_t> Rank;
extern llvm::cl::opt<uint32_t> ProtocolKind;
extern llvm::cl::opt<uint32_t> Field;
extern llvm::cl::opt<bool> EngineTrace;

const uint32_t STATUS_OK = 200;
const uint32_t STATUS_ERROR_DEFAULT = 201;
//enum class ServiceStatus : uint32_t


void TryConnectToMesh(std::shared_ptr<yacl::link::Context> lctx,std::size_t retry_time );
//
std::shared_ptr<yacl::link::Context> MakeLink(const std::string& parties,
                                              size_t rank,const GlobalConfig& global_config,std::vector<std::string>& members);

// Create an evaluator with setting flags.
std::unique_ptr<spu::HalContext> MakeHalContext(const std::string& parties,size_t rank,const GlobalConfig& global_config,std::vector<std::string>& members);

std::string CreateParties(bool testLocal,const GlobalConfig& global_config,uint32_t self_rank,std::vector<std::string>& ips);
}
}
