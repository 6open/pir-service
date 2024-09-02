#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "config.h"

TEST(config, default_test) 
{
    GlobalConfig& config =  global_config_;
    EXPECT_EQ(config.proxy_config_.proxy_mode_,0);
    EXPECT_EQ(config.proxy_config_.gateway_config_.port_,12000);
    EXPECT_EQ(config.grpc_config_.self_port_,12600);
    EXPECT_EQ(config.grpc_config_.other_port_,12600);
    EXPECT_EQ(config.log_config_.log_path_,"/home/admin/log");
}

TEST(config, log) 
{
    //just log
    json j1;
    j1["logConfig"]["path"] = "haha";
    GlobalConfig config = j1.get<GlobalConfig>();
    EXPECT_EQ(config.log_config_.log_path_,"haha");
    EXPECT_EQ(config.proxy_config_.proxy_mode_,0);
    EXPECT_EQ(config.proxy_config_.gateway_config_.port_,12000);
    EXPECT_EQ(config.grpc_config_.self_port_,12600);
    EXPECT_EQ(config.grpc_config_.other_port_,12600);
}
TEST(config, InitGlobalConfig) 
{
    json j1;
    j1["logConfig"]["path"] = "/home/admin/log";
    j1["proxyConfig"]["proxyMode"]  = 99;
    j1["proxyConfig"]["gatewayConfig"]["port"]  = 123;
    j1["grpcConfig"]["selfPort"]  = 234;
    j1["grpcConfig"]["otherPort"]  = 234;
    GlobalConfig config = j1.get<GlobalConfig>();
    EXPECT_EQ(config.log_config_.log_path_,"/home/admin/log");
    EXPECT_EQ(config.proxy_config_.proxy_mode_,99);
    EXPECT_EQ(config.proxy_config_.gateway_config_.port_,123);
    EXPECT_EQ(config.grpc_config_.self_port_,234);
    EXPECT_EQ(config.grpc_config_.other_port_,234);
}