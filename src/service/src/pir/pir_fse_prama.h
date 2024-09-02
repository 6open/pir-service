// Copyright 2019 Ant Group Co., Ltd
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
#include <string>
#include <string_view>

const std::string kPirFseParma{ R"({
            "table_params": {
                "hash_func_count": 3,
                "table_size": 512,
                "max_items_per_bin": 92
            },
            "item_params": {
                "felts_per_item": 8
            },
            "query_params": {
                "ps_low_degree": 0,
                "query_powers": [ 3, 4, 5, 8, 14, 20, 26, 32, 38, 41, 42, 43, 45, 46 ]
            },
            "seal_params": {
                "plain_modulus": 40961,
                "poly_modulus_degree": 4096,
                "coeff_modulus_bits": [ 49, 40, 20 ]
            }
        })"};

const std::string kPirFseClientParma{ R"({
            "table_params": {
                "hash_func_count": 3,
                "table_size": 512,
                "max_items_per_bin": 92
            },
            "item_params": {
                "felts_per_item": 8
            },
            "query_params": {
                "ps_low_degree": 0,
                "query_powers": [1, 4, 5, 15, 18, 27, 34]
            },
            "seal_params": {
                "plain_modulus": 40961,
                "poly_modulus_degree": 4096,
                "coeff_modulus_bits": [ 49, 40, 20 ]
            }
        })"};