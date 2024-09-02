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

#include<map>
#include "db_client.h"
#include "db_server.h"
#include "batch_writer.h"

#include <gtest/gtest.h>
class DbClientServerTest : public ::testing::Test 
{
public:

    void ServerSend(const std::string& key,const std::string& message)
    {
        std::unique_lock<std::mutex> lock(server_mu);
        server_sender[key] = message;
        server_con.notify_one();
    }
    std::string ClientRecv(const std::string& key)
    {
        std::string value;
        std::unique_lock<std::mutex> lock(server_mu);
        auto stop_waiting = [&] {
      // SPDLOG_INFO("in stop_waiting msg_key:{}",msg_key);
            auto itr = this->server_sender.find(key);
            if (itr == this->server_sender.end()) {
                return false;
            } else {
                value = std::move(itr->second);
                this->server_sender.erase(itr);
                return true;
            }
        };
        while (!stop_waiting()) {
        //                              timeout_us
        // SPDLOG_INFO("stop_waiting msg_key:{}",msg_key);

        if (server_con.wait_for(lock,  std::chrono::seconds(recv_timeout_ms_)) == std::cv_status::timeout) {
                std::cout<<"cleint get message timeout key="<<key<<endl;
                return value;
            }
        }

        return value;
    }
    void ClientSend(const std::string& key,const std::string& message)
    {
        std::unique_lock<std::mutex> lock(client_mu);
        client_sender[key] = message;
        client_con.notify_one();
    }
    std::string ServerRecv(const std::string& key)
    {
        std::string value;
        std::unique_lock<std::mutex> lock(client_mu);
        auto stop_waiting = [&] {
      // SPDLOG_INFO("in stop_waiting msg_key:{}",msg_key);
            auto itr = this->client_sender.find(key);
            if (itr == this->client_sender.end()) {
                return false;
            } else {
                value= std::move(itr->second);
                this->client_sender.erase(itr);
                return true;
            }
        };
        while (!stop_waiting()) {
        //                              timeout_us
        // SPDLOG_INFO("stop_waiting msg_key:{}",msg_key);

        if (client_con.wait_for(lock, std::chrono::seconds(recv_timeout_ms_)) == std::cv_status::timeout) {
                std::cout<<"server get message timeout key="<<key<<endl;
                return value;
            }
        }

        return value;
    }
    std::map<std::string,std::string> server_sender;
    std::mutex server_mu;
    std::condition_variable server_con;
    std::map<std::string,std::string> client_sender;
    std::mutex client_mu;
    std::condition_variable client_con;
    std::size_t recv_timeout_ms_ = 2;
};

TEST_F(DbClientServerTest, stoull)
{
    std::size_t i = 10;
    std::string str = std::to_string(i);
    std::size_t j = std::stoull(str);
    EXPECT_EQ(i,j);
}
TEST_F(DbClientServerTest, normal)
{
    std::string key("sjajd");
    std::string meta_dir("/home/admin/data");
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
    DbMetaManager::Instance().SetMetaDir(meta_dir);
    DbMeta& dm = *DbMetaManager::Instance().GetDbMeta(key,data_path,key_columns_,label_columns_);
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

    std::string client_name = meta_dir+"/hihihe";
    std::map<std::string,std::string> ret;
    auto create_client = [&]()
    {
        BatchWriter bwriter2(client_name,key_columns_,label_columns_);
        for(std::size_t j=0;j<10;++j)
        {
            int id = 1000+j*10;
            std::string ids = std::to_string(id);
            std::string label = std::to_string(id);
            ret[ids] = label;
            bwriter2.AddItem(ids,label);
        }
        bwriter2.Release();
    };
    create_client();

    auto server_sender = [&](const std::string& mkey,const std::string& message)
    {
        ServerSend(mkey,message);
    };
    auto client_sender = [&](const std::string& mkey,const std::string& message)
    {
        ClientSend(mkey,message);
    };
    auto server_recv = [&](const std::string& mkey) ->std::string
    {
        return ServerRecv(mkey);
    };
    auto client_recv = [&](const std::string& mkey) ->std::string
    {
        return ClientRecv(mkey);
    };

    std::string task_id("task_id");
    auto server_func = [&]()
    {
        DbServer server(key,task_id,server_sender,server_recv);
        server.Run();
    };
    std::string result_path = meta_dir+"/result_hhisadh";
    auto client_func = [&]()
    {
        DbClient client(client_name,result_path,task_id,key_columns_,label_columns_,client_sender,client_recv);
        client.Run();
    };
    std::thread server_thread(server_func);
    std::thread client_thread(client_func);
    server_thread.join();
    client_thread.join();

    using namespace yacl::io;
    std::unique_ptr<InputStream> in(new FileInputStream(result_path));
    Schema s;
    for(const std::string& str : key_columns_)
    {
        s.feature_types.push_back(Schema::STRING);
        s.feature_names.push_back(str);
    }
    for(const std::string& str : label_columns_)
    {
        s.feature_types.push_back(Schema::STRING);
        s.feature_names.push_back(str);
    }
    ReaderOptions r_ops;
    //r_ops.column_reader = true;
    r_ops.file_schema = s;
    r_ops.batch_size = 1000;
    r_ops.use_header_order = true;
    r_ops.column_reader = false;

    CsvReader reader(r_ops, std::move(in));
    ColumnVectorBatch batch;
    reader.Init();
    while(reader.Next(r_ops.batch_size,&batch))
    {
        for(std::size_t i=0;i<batch.Shape().rows;++i)
        {
            std::string str;
            for(std::size_t j=1;j<batch.Shape().cols;++j)
            {
                str += batch.At<std::string>(i, j)+",";
            }
            str.pop_back();
            std::string key_id = batch.At<std::string>(i, 0);
            auto iter =  ret.find(key_id);
            EXPECT_TRUE(iter != ret.end());
            auto pos = str.find(iter->second);
            EXPECT_EQ(pos,0);
            ret.erase(iter);
        }
    }
    EXPECT_TRUE(ret.empty());
}

/*
bazel test //src/kernel/data_db:db_server_client_test --test_filter=DbClientServerTest.client_filter
*/
TEST_F(DbClientServerTest, client_filter)
{
    std::string key("sjajd");
    std::string meta_dir("/home/admin/data");
    std::string data_path("hehe");
    std::vector<std::string> key_columns_ = {"id1"};
    std::vector<std::string> label_columns_ = {"la1","la2","la3"};
    std::vector<std::string> client_label_columns_ = {"la2"};
    BatchWriter bwriter(data_path,key_columns_,label_columns_);
    for(std::size_t i=0;i<100;++i)
    {
        int id = 1000+i;
        std::string ids = std::to_string(id);
        std::string label = std::to_string(id);
        for(std::size_t j = 0;j<label_columns_.size();++j)
        {
            label += label_columns_[j]+"_"+ids+",";
        }
        label.pop_back();
        bwriter.AddItem(ids,label);
    }
    bwriter.Release();
    cout<<"write ok"<<endl;
    EXPECT_TRUE(std::filesystem::exists(data_path));
    DbMetaManager::Instance().SetMetaDir(meta_dir);
    DbMeta& dm = *DbMetaManager::Instance().GetDbMeta(key,data_path,key_columns_,label_columns_);
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

    std::string client_name = meta_dir+"/hihihe";
    std::map<std::string,std::string> ret;
    auto create_client = [&]()
    {
        BatchWriter bwriter2(client_name,key_columns_,{"hihihi"});
        for(std::size_t j=0;j<10;++j)
        {
            int id = 1000+j*10;
            std::string ids = std::to_string(id);
            std::string label = std::to_string(id);
            ret[ids] = label;
            bwriter2.AddItem(ids,label);
        }
        bwriter2.Release();
    };
    create_client();

    auto server_sender = [&](const std::string& mkey,const std::string& message)
    {
        ServerSend(mkey,message);
    };
    auto client_sender = [&](const std::string& mkey,const std::string& message)
    {
        ClientSend(mkey,message);
    };
    auto server_recv = [&](const std::string& mkey) ->std::string
    {
        return ServerRecv(mkey);
    };
    auto client_recv = [&](const std::string& mkey) ->std::string
    {
        return ClientRecv(mkey);
    };

    std::string task_id("task_id");
    auto server_func = [&]()
    {
        DbServer server(key,task_id,server_sender,server_recv);
        server.Run();
    };
    std::string result_path = meta_dir+"/result_hhisadh";
    auto client_func = [&]()
    {
        DbClient client(client_name,result_path,task_id,key_columns_,client_label_columns_,client_sender,client_recv);
        client.Run();
    };
    std::thread server_thread(server_func);
    std::thread client_thread(client_func);
    server_thread.join();
    client_thread.join();
    std::ifstream inputFile(result_path);
    EXPECT_EQ(inputFile.is_open(),true);
    std::string line;
    std::getline(inputFile, line);
    std::size_t j=0;
    while(std::getline(inputFile, line))
    {
        std::vector<absl::string_view> result = absl::StrSplit(line, ',');
        EXPECT_EQ(result.size(),2);
        // std::string key_value = "id"+std::to_string(j);
        // EXPECT_EQ(result[0],key_value);
        auto iter =  ret.find(std::string(result[0]));
        EXPECT_TRUE(iter != ret.end());
        ret.erase(iter);
        std::string label_value = "la2_"+std::string(result[0]);
        EXPECT_EQ(result[1],label_value);
        ++j;
    }

    EXPECT_TRUE(ret.empty());
}