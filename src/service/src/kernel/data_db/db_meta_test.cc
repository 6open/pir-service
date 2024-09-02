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
#include "batch_writer.h"
#include "db_meta.h"
#include "gtest/gtest.h"
#include <thread>
#include <cstdio>
#include <filesystem>
// using namespace yacl::io;
using namespace std;

class DbMetaTest : public ::testing::Test 
{
public:
    virtual void SetUp() override
    {
        std::filesystem::remove_all(meta_dir_);
        if (!std::filesystem::exists(meta_dir_)) {
            std::filesystem::create_directory(meta_dir_);
        }
    }
    

    std::unique_ptr<DbMeta> CreateDbMeta(const std::string& key)
    {
        // std::string key("sjajd123");
        std::string meta_dir(meta_dir_);
        std::string data_path("hehe");
        std::vector<std::string> key_columns_ = {"id1"};
        std::vector<std::string> label_columns_ = {"la1"};
        BatchWriter bwriter(data_path,key_columns_,label_columns_);
        for(std::size_t j=0;j<100;++j)
        {
            int id = 1000+j;
            std::string ids = std::to_string(id);
            std::string label = std::to_string(id);
            bwriter.AddItem(ids,label);
        }
        bwriter.Release();
        cout<<"write ok key="<<key<<endl;
        EXPECT_TRUE(std::filesystem::exists(data_path));
        auto dm_ptr = std::make_unique<DbMeta>(key,meta_dir,data_path,key_columns_,label_columns_);
        DbMeta& dm = *dm_ptr.get();
        EXPECT_FALSE(dm.IsSetup());
        dm.Setup();
        EXPECT_TRUE(dm.IsSetup());
        EXPECT_TRUE(dm.sender_cacher_ != nullptr);
        DbMetaInfo& info = dm.sender_cacher_->meta_info_;
        EXPECT_EQ(dm.sender_cacher_->data_caches_.size(),info.mega_batchs_.size());
        for(const MegaBatch& mb1: info.mega_batchs_)
        {
            EXPECT_TRUE(std::filesystem::exists(mb1.DataCacheName()));
        }
        return dm_ptr;
    }
    std::string meta_dir_ = "/home/admin/data/utmeta";
};

TEST_F(DbMetaTest, GetBatchCount)
{
    std::string str("aaaaaaaaaaa");
    std::size_t bsize = 1000000;
    EXPECT_EQ(DbMeta::GetBatchCount(str,bsize) < bsize,true);

    std::string str1("hsadghagsdhasg");
    EXPECT_EQ(DbMeta::GetBatchCount(str1,bsize) < bsize,true);
    std::string str2("hs");
    EXPECT_EQ(DbMeta::GetBatchCount(str2,bsize) < bsize,true);
}

TEST_F(DbMetaTest, normal_test)
{
    std::string key("sjajd");
    std::string meta_dir(meta_dir_);
    std::string data_path("hehe");
    std::vector<std::string> key_columns_ = {"id1"};
    std::vector<std::string> label_columns_ = {"la1"};
    BatchWriter bwriter(data_path,key_columns_,label_columns_);
    for(std::size_t j=0;j<100;++j)
    {
        int id = 1000+j;
        std::string ids = std::to_string(id);
        std::string label = std::to_string(id);
        bwriter.AddItem(ids,label);
    }
    bwriter.Release();
    cout<<"write ok"<<endl;
    EXPECT_TRUE(std::filesystem::exists(data_path));
    DbMeta dm(key,meta_dir,data_path,key_columns_,label_columns_);
    EXPECT_FALSE(dm.IsSetup());
    dm.Setup();
    EXPECT_TRUE(dm.IsSetup());
    EXPECT_TRUE(dm.sender_cacher_ != nullptr);
    DbMetaInfo& info = dm.sender_cacher_->meta_info_;
    EXPECT_EQ(dm.sender_cacher_->data_caches_.size(),info.mega_batchs_.size());
    for(const MegaBatch& mb1: info.mega_batchs_)
    {
        EXPECT_TRUE(std::filesystem::exists(mb1.DataCacheName()));
    }
}

TEST_F(DbMetaTest, bigdata)
{
    std::size_t num = 100000;
    // num = 4000000;
    std::string key = ("sjajd")+std::to_string(num);
    std::string meta_dir(meta_dir_);
    std::string data_path("hehe");
    std::vector<std::string> key_columns_ = {"id1"};
    std::vector<std::string> label_columns_ = {"la1"};
    BatchWriter bwriter(data_path,key_columns_,label_columns_);
    for(std::size_t j=0;j<num;++j)
    {
        int id = 1000+j;
        std::string ids = std::to_string(id);
        std::string label = std::to_string(id);
        bwriter.AddItem(ids,label);
    }
    bwriter.Release();
    cout<<"write ok"<<endl;
    EXPECT_TRUE(std::filesystem::exists(data_path));
    DbMeta dm(key,meta_dir,data_path,key_columns_,label_columns_);
    EXPECT_FALSE(dm.IsSetup());
    dm.Setup();
    EXPECT_TRUE(dm.IsSetup());
    EXPECT_TRUE(dm.sender_cacher_ != nullptr);
    DbMetaInfo& info = dm.sender_cacher_->meta_info_;
    EXPECT_EQ(dm.sender_cacher_->data_caches_.size(),info.mega_batchs_.size());
    for(const MegaBatch& mb1: info.mega_batchs_)
    {
        EXPECT_TRUE(std::filesystem::exists(mb1.DataCacheName()));
    }
}

/*
bazel test //src/kernel/data_db:db_meta_test --test_filter=DbMetaTest.testLoacCache
*/
TEST_F(DbMetaTest, testLoacCache)
{
    std::string key("sjajd123");
    std::string meta_dir(meta_dir_);
    std::string data_path("hehe");
    std::vector<std::string> key_columns_ = {"id1"};
    std::vector<std::string> label_columns_ = {"la1"};
    BatchWriter bwriter(data_path,key_columns_,label_columns_);
    for(std::size_t j=0;j<100;++j)
    {
        int id = 1000+j;
        std::string ids = std::to_string(id);
        std::string label = std::to_string(id);
        bwriter.AddItem(ids,label);
    }
    bwriter.Release();
    cout<<"write ok"<<endl;
    EXPECT_TRUE(std::filesystem::exists(data_path));
    DbMeta dm(key,meta_dir,data_path,key_columns_,label_columns_);
    EXPECT_FALSE(dm.IsSetup());
    dm.Setup();
    EXPECT_TRUE(dm.IsSetup());
    EXPECT_TRUE(dm.sender_cacher_ != nullptr);
    DbMetaInfo& info = dm.sender_cacher_->meta_info_;
    EXPECT_EQ(dm.sender_cacher_->data_caches_.size(),info.mega_batchs_.size());
    for(const MegaBatch& mb1: info.mega_batchs_)
    {
        EXPECT_TRUE(std::filesystem::exists(mb1.DataCacheName()));
    }

    std::string meta_name = dm.meta_info_.meta_path_;
    auto mp = DbMeta::LoadCahce(meta_name);
    EXPECT_TRUE(mp != nullptr);
    EXPECT_EQ(mp->meta_info_,dm.meta_info_);
    EXPECT_EQ(mp->meta_info_.mega_batchs_.size() ,1);
    EXPECT_TRUE(std::filesystem::remove(mp->meta_info_.mega_batchs_[0].DataCacheName()));
    auto mp2 = DbMeta::LoadCahce(meta_name);
    EXPECT_TRUE(mp2 == nullptr);
    //test move;
    DbMetaInfo info2 = mp->meta_info_;
    auto mp3 = std::move(mp);
    EXPECT_TRUE(mp3!=nullptr);
    EXPECT_EQ(info2,mp3->meta_info_);
}

/*
bazel test //src/kernel/data_db:db_meta_test --test_filter=DbMetaTest.managerLoad
*/
TEST_F(DbMetaTest, managerLoad)
{
    DbMetaManager::Instance().SetMetaDir(meta_dir_);
    DbMetaManager::Instance().LoadingCache();
    auto& metas = DbMetaManager::Instance().GetMetas();
    EXPECT_EQ(metas.size(),0);
    for(int i=0;i<10;++i)
    {
        std::string key = "dad"+std::to_string(i);
        CreateDbMeta(key);
    }
    DbMetaManager::Instance().LoadingCache();
    EXPECT_EQ(metas.size(),10);
}
