
// Copyright 2019 Ant Group Co., Ltd.
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

#include "yacl/link/factory.h"
namespace yacl::link {
// class FactoryBrpcGateway : public yacl::link::ILinkFactory {
//  public:
//   std::shared_ptr<Context> CreateContext(const ContextDesc& desc,
//                                          size_t self_rank) override;
// };
    class FactoryBrpcGateway {
      public:
        std::shared_ptr<Context> CreateContextGateway(const ContextDesc& desc,size_t self_rank,const std::vector<std::string>& member_ids);
    };
}