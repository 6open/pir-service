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

// #include "src/pir/pir_manager.h"
// #include "batch_writer.h"
// #include "db_meta.h"

#include "data_cacher.h"
#include "batch_writer.h"
#include "gtest/gtest.h"
#include <thread>
#include <cstdio>
#include <filesystem>
#include "src/pir/pir_fse_prama.h"

// using namespace yacl::io;
using namespace std;
namespace fs = std::filesystem;
class DbCacherTest : public ::testing::Test 
{
public:
    std::vector<std::string> key_columns_ = {"id1"};
    std::vector<std::string> label_columns_ = {"la1"};
};
/*
bazel test //src/kernel/data_db:data_cacher_test --test_filter=DbCacherTest.SaveAndLoad_simplethread
*/

TEST_F(DbCacherTest,SaveAndLoad_simplethread)
{
    MegaBatch mb;
    mb.data_cache_name_ = "data_cache";
    if(std::filesystem::exists(mb.data_cache_name_))
    {
        std::filesystem::remove(mb.data_cache_name_);
    }
    EXPECT_FALSE(std::filesystem::exists(mb.data_cache_name_));
    std::size_t num = 10;
    for(std::size_t i=0;i<num;++i)
    {
        std::string filename = "haha"+std::to_string(i);
        if(std::filesystem::exists(filename))
        {
            std::filesystem::remove(filename);
        }
        EXPECT_FALSE(std::filesystem::exists(filename));
        BatchWriter bwriter(filename,key_columns_,label_columns_);
        for(std::size_t j=0;j<100;++j)
        {
            int id = i*1000+j;
            std::string ids = std::to_string(id);
            std::string label = std::to_string(id);
            bwriter.AddItem(ids,label);
        }
        bwriter.Release();
        EXPECT_TRUE(std::filesystem::exists(filename));
        MegaBatch mb2(filename,100,i);
        mb += mb2;
    }
    SeDataCacher se_data_cacher(mb,key_columns_,label_columns_);
    se_data_cacher.SetupImpl();
    cout<<"cache name is "<<mb.data_cache_name_<<endl;
    EXPECT_TRUE(std::filesystem::exists(mb.data_cache_name_));
    se_data_cacher.LoadingImpl();
    EXPECT_FALSE(se_data_cacher.server_wrapper_ == nullptr);
    apsi::wrapper::APSIServerWrapper& swrapper = *se_data_cacher.server_wrapper_.get();
    std::string client_parma = kPirFseParma;
    apsi::wrapper::APSIClientWrapper cwrapper(client_parma);
    for(std::size_t i=0;i<num;++i)
    {
        for(std::size_t j=0;j< 100;++j)
        {
            int id = i*1000+j;
            std::string ids = std::to_string(id);
            std::vector<std::string> c_input_items = { ids, "pear", "egg", "item4"};
            std::string oprf_request = cwrapper.oprf_request(c_input_items);
            std::string oprf_response = swrapper.handle_oprf_request(oprf_request);

            // Step 3: 调用 build_query 方法，并获取返回值
            std::string query_string = cwrapper.build_query(oprf_response);

            //std::string response_string = server.handle_query(query_string);
            std::string response_string = swrapper.handle_query(query_string);

            // Step 4: 调用 extract_labeled_result 方法，并获取返回值
            vector<string> intersection = cwrapper.extract_labeled_result(response_string);
            EXPECT_EQ(intersection.size(),c_input_items.size());
            cout<<"i="<<i<<",j="<<j<<",intersection[0] is"<<intersection[0]<<endl;
            auto pos = intersection[0].find(ids);
            EXPECT_EQ(pos,0);
        }
    }
}

/*
bazel test //src/kernel/data_db:data_cacher_test --test_filter=DbCacherTest.SaveAndLoad_multithread
*/
TEST_F(DbCacherTest, SaveAndLoad_multithread)
{
    MegaBatch mb;
    mb.data_cache_name_ = "data_cache";
    if(std::filesystem::exists(mb.data_cache_name_))
    {
        std::filesystem::remove(mb.data_cache_name_);
    }
    EXPECT_FALSE(std::filesystem::exists(mb.data_cache_name_));
    std::size_t num = 10;
    for(std::size_t i=0;i<num;++i)
    {
        std::string filename = "haha"+std::to_string(i);
        if(std::filesystem::exists(filename))
        {
            std::filesystem::remove(filename);
        }
        EXPECT_FALSE(std::filesystem::exists(filename));
        BatchWriter bwriter(filename,key_columns_,label_columns_);
        for(std::size_t j=0;j<100;++j)
        {
            int id = i*1000+j;
            std::string ids = std::to_string(id);
            std::string label = std::to_string(id);
            bwriter.AddItem(ids,label);
        }
        bwriter.Release();
        EXPECT_TRUE(std::filesystem::exists(filename));
        MegaBatch mb2(filename,100,i);
        mb += mb2;
    }
    SeDataCacher se_data_cacher(mb,key_columns_,label_columns_);
    se_data_cacher.SetupImpl();
    cout<<"cache name is "<<mb.data_cache_name_<<endl;
    EXPECT_TRUE(std::filesystem::exists(mb.data_cache_name_));
    se_data_cacher.LoadingImpl();
    EXPECT_FALSE(se_data_cacher.server_wrapper_ == nullptr);
    apsi::wrapper::APSIServerWrapper& swrapper = *se_data_cacher.server_wrapper_.get();
    std::string client_parma = kPirFseParma;
    apsi::wrapper::APSIClientWrapper cwrapper(client_parma);
    auto func = [&swrapper,&cwrapper](int i)
    {
        for(std::size_t j=0;j<1;++j)
        {
            // i = 3;
            int id = i*1000+j;
            std::string ids = std::to_string(id);
            std::vector<std::string> c_input_items = { ids, "pear", "egg", "item4"};
            std::string oprf_request = cwrapper.oprf_request(c_input_items);
            std::string oprf_response = swrapper.handle_oprf_request(oprf_request);

            // Step 3: 调用 build_query 方法，并获取返回值
            std::string query_string = cwrapper.build_query(oprf_response);

            //std::string response_string = server.handle_query(query_string);
            std::string response_string = swrapper.handle_query(query_string);

            // Step 4: 调用 extract_labeled_result 方法，并获取返回值
            vector<string> intersection = cwrapper.extract_labeled_result(response_string);
            EXPECT_EQ(intersection.size(),c_input_items.size());
            cout<<"i="<<i<<",j="<<j<<",intersection[0] is"<<intersection[0]<<endl;
            auto pos = intersection[0].find(ids);
            if(j<100)
                EXPECT_EQ(pos,0);
            else
                EXPECT_EQ(pos,std::string::npos);

        }
    };
    std::vector<std::thread> threads;
    num = 10;
    for(std::size_t i=0;i<num;++i)
    {
        threads.push_back(std::thread(func,i));
    }
    for(std::size_t i=0;i<num;++i)
    {
        threads[i].join();
    }
    std::filesystem::remove(mb.data_cache_name_);
}

/*
bazel test //src/kernel/data_db:data_cacher_test --test_filter=DbCacherTest.LoadErrorFile
*/
TEST_F(DbCacherTest, LoadErrorFile)
{
    MegaBatch mb;
    mb.data_cache_name_ = "data_cache";
    if(std::filesystem::exists(mb.data_cache_name_))
    {
        std::filesystem::remove(mb.data_cache_name_);
    }
    EXPECT_FALSE(std::filesystem::exists(mb.data_cache_name_));
    std::ofstream out(mb.data_cache_name_);
    EXPECT_TRUE(out.is_open());
    out<<"hihi heha haha"<<std::endl;
    out.close();
    SeDataCacher se_data_cacher(mb,key_columns_,label_columns_);
    try {
        se_data_cacher.Loading();
    } catch(std::exception& e)
    {
        std::cout<<e.what()<<std::endl;
        se_data_cacher.Release();
    }
    std::cout<<"asdasda"<<std::endl;
    std::filesystem::remove(mb.data_cache_name_);
}
