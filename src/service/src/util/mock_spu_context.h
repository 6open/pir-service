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
#include <random>

#include "yacl/link/link.h"

#include "libspu/core/trace.h"

#include "libspu/spu.pb.h"

#include "libspu/core/type_util.h"

namespace spu {

// The hal evaluation context for all spu operators.
class HalContext final {
  RuntimeConfig rt_config_;

  std::shared_ptr<yacl::link::Context> lctx_;

  std::default_random_engine rand_engine_;

  HalContext() = default;

 public:
  explicit HalContext(const RuntimeConfig& config,
                      const std::shared_ptr<yacl::link::Context>& lctx);

  HalContext(const HalContext& other) = delete;
  HalContext& operator=(const HalContext& other) = delete;

  // all parties get a 'corresponding' hal context when forked.
  std::unique_ptr<HalContext> fork();

  HalContext(HalContext&& other) = default;

  //
  const std::shared_ptr<yacl::link::Context>& lctx() const { return lctx_; }

  // Return current working fixed point fractional bits.
  size_t getFxpBits() const { return getDefaultFxpBits(rt_config_); }

  // Return current working field of MPC engine.
  FieldType getField() const { return rt_config_.field(); }

  // Return current working runtime config.
  const RuntimeConfig& rt_config() const { return rt_config_; }

  //
  std::default_random_engine& rand_engine() { return rand_engine_; }
};

}  // namespace spu
