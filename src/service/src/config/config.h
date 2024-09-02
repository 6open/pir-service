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
#include<iostream>
#include<string>

#include "src/util/json.h"
/*
更新日志：
1k-0.0.7 : 强制检查mlflow变量
1k-0.0.8 : 移除缓存文件上传mlflow，改为加密容器
1k-0.0.9 : 清除结果文件结尾的\0字符
*/


#define MPC_PIR_VERSION "1k-0.0.9"

struct LogConfig
{
    std::string log_path_ = "/home/admin/log";
};

struct DataConfig 
{
    std::string source_data_path_ = "/data/storage/dataset/pre_deal_result";
    // std::string output_data_path_ = "/data/storage/dataset/pre_deal_result/pir_result";
    std::string output_data_path_ = "/data/pir_meta/pir_result";
};
struct GrpcConfig 
{
    uint32_t self_port_ = 12600;
    uint32_t other_port_ = 12600;
};

struct  PsiConfig
{
    /* data */
};

struct  PirConfig
{
    std::string oprf_key_path_ = "/home/admin/pir/oprf_key";
    std::string apsi_setup_path_ = "/home/admin/pir/setup";
    // std::string data_meta_path = "/data/storage/dataset/pre_deal_result/pir_meta";
    std::string data_meta_path = "/data/pir_meta";
    std::string default_algo_ = "SE";
    uint32_t count_per_query_ = 256;
    uint32_t max_label_length_ = 32;
};

struct GatewayConfig
{
    uint32_t port_ = 12000;
};

struct ProxyConfig
{
    uint32_t proxy_mode_ = 0;//0
    GatewayConfig gateway_config_;
};
struct HttpConfig 
{
    uint32_t port_ = 12601;
};

struct GlobalConfig
{
    /* data */
    LogConfig log_config_;
    DataConfig data_config_;
    PsiConfig psi_config_;
    PirConfig pir_config_;
    ProxyConfig proxy_config_;
    GrpcConfig grpc_config_;
    HttpConfig http_config_;
};

extern GlobalConfig global_config_;
extern GlobalConfig& InitGlobalConfig(const std::string& file_path);
extern GlobalConfig& GetGlobalConfig();
void from_json(const json& j, LogConfig& p);
void from_json(const json& j, DataConfig& p);

void from_json(const json& j, GatewayConfig& p);
void from_json(const json& j, ProxyConfig& p);
void from_json(const json& j, GrpcConfig& p);
void from_json(const json& j, HttpConfig& p);


void from_json(const json& j, PsiConfig& p);
void from_json(const json& j, PirConfig& p);


void from_json(const json& j, GlobalConfig& p);