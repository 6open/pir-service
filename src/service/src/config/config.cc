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
#include "config.h"

GlobalConfig global_config_;

GlobalConfig& InitGlobalConfig(const std::string& file_path)
{
    json data = JsonFromFile(file_path);
    global_config_ = data.get<GlobalConfig>();
    return global_config_;
}
GlobalConfig& GetGlobalConfig()
{
    return global_config_;
}


void from_json(const json& j, LogConfig& p)
{
    j.at("path").get_to(p.log_path_);
}

void from_json(const json& j, DataConfig& p)
{
    j.at("sourceDataPath").get_to(p.source_data_path_);
    j.at("outputDataPath").get_to(p.output_data_path_);
}

void from_json(const json& j, GatewayConfig& p)
{
    j.at("port").get_to(p.port_);
}

void from_json(const json& j, ProxyConfig& p)
{
    j.at("proxyMode").get_to(p.proxy_mode_);
    j.at("gatewayConfig").get_to(p.gateway_config_);
}

void from_json(const json& j, GrpcConfig& p)
{
    j.at("selfPort").get_to(p.self_port_);
    j.at("otherPort").get_to(p.other_port_);
}

void from_json(const json& j, HttpConfig& p)
{
    j.at("port").get_to(p.port_);
}

void from_json(const json& j, PsiConfig& p)
{

}

void from_json(const json& j, PirConfig& p)
{
    j.at("oprfKeyPath").get_to(p.oprf_key_path_);
    j.at("apsiSetupPath").get_to(p.apsi_setup_path_);
    j.at("dataMetaPath").get_to(p.data_meta_path);
    j.at("defaultAlgo").get_to(p.default_algo_);
    j.at("countPerQuery").get_to(p.count_per_query_);
    j.at("maxLabelLength").get_to(p.max_label_length_);
}

void from_json(const json& j, GlobalConfig& p)
{
    if(j.contains("logConfig"))
        j.at("logConfig").get_to(p.log_config_);
    if(j.contains("dataConfig"))
        j.at("dataConfig").get_to(p.data_config_);
    if(j.contains("proxyConfig"))
        j.at("proxyConfig").get_to(p.proxy_config_);
    if(j.contains("grpcConfig"))
        j.at("grpcConfig").get_to(p.grpc_config_);
    if(j.contains("psiConfig"))
        j.at("psiConfig").get_to(p.psi_config_);
    if(j.contains("pirConfig"))
        j.at("pirConfig").get_to(p.pir_config_);
}