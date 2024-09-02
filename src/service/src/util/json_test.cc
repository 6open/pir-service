
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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "json.h"
TEST(json, Normal_Empty) 
{
    json ex1 = json::parse(R"(
    {
        "pi": 3.141,
        "hello":"hello world",
        "happy": true
    }
    )");
    EXPECT_EQ(ex1["happy"],true);
    EXPECT_EQ(ex1["pi"],3.141);
    EXPECT_EQ(JsonGetInt(ex1,"pi",-1),3);
    EXPECT_EQ(JsonGetIntNoCast(ex1,"pi",-1),-1);
    EXPECT_EQ(JsonGetString(ex1,"hello"),"hello world");
    EXPECT_EQ(JsonGetString(ex1,"pi"),"");
}
TEST(json, write_config) 
{
    json j1;
    j1["logConfig"]["path"] = "/home/admin/log";
    j1["proxyConfig"]["proxyMode"]  = 1;
    j1["proxyConfig"]["gatewayConfig"]["port"]  = 12000;
    j1["grpcConfig"]["selfPort"]  = 12600;
    j1["grpcConfig"]["otherPort"]  = 12600;
    JsonToFile(j1,"/home/admin/mpc_config.json");

    json j2 = JsonFromFile("/home/admin/mpc_config.json");
    EXPECT_EQ(j2["logConfig"]["path"],"/home/admin/log");
    EXPECT_EQ(j2["proxyConfig"]["proxyMode"],1);
}
