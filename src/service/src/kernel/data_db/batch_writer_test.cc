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
#include "gtest/gtest.h"
#include <thread>
#include <cstdio>
#include "yacl/io/rw/csv_reader.h"
#include "yacl/io/rw/csv_writer.h"
#include "yacl/io/rw/schema.h"
#include "yacl/io/stream/file_io.h"
#include "spdlog/spdlog.h"
#include "libspu/psi/utils/batch_provider.h"

using namespace yacl::io;
using namespace std;

class BatchWriterTest : public ::testing::Test {};

TEST_F(BatchWriterTest, header)
{
    std::string file_path("haha.csv");
    std::vector<std::string> fileds{"id"};
    std::vector<std::string> labels{"label1","label2","label3"};
    BatchWriter batch_writer(file_path,fileds,labels);
    batch_writer.Release();
    std::unique_ptr<InputStream> in(new FileInputStream(file_path));
//   Schema s;
//   s.feature_types = {Schema::STRING, Schema::FLOAT, Schema::FLOAT};
//   s.feature_names = {"id", "f2", "label"};
    ReaderOptions r_ops;
    Schema s;
    s.feature_types.insert(s.feature_types.end(),fileds.size()+labels.size(),Schema::STRING);
    // s.feature_types = {Schema::STRING,Schema::FLOAT,Schema::DOUBLE};
    s.feature_names.insert(s.feature_names.end(),fileds.begin(),fileds.end());
    s.feature_names.insert(s.feature_names.end(),labels.begin(),labels.end());

    r_ops.file_schema = s;
    r_ops.batch_size = 1;
    r_ops.use_header_order = true;


    CsvReader reader(r_ops, std::move(in));
    reader.Init();
    std::size_t i = 0;
    EXPECT_EQ(fileds.size()+labels.size(),reader.Headers().size());
    for(const std::string& str : fileds)
    {
        EXPECT_EQ(str,reader.Headers()[i++]);
    }
    for(const std::string& str : labels)
    {
        EXPECT_EQ(str,reader.Headers()[i++]);
    }
    EXPECT_EQ(std::remove(file_path.c_str()),0);
}

TEST_F(BatchWriterTest, normal)
{
    std::string file_path("haha.csv");
    std::vector<std::string> fileds{"id"};
    std::vector<std::string> labels{"label1","label2","label3"};
    BatchWriter batch_writer(file_path,fileds,labels);
    std::size_t num = 5000;
    for(std::size_t i = 0;i<num;++i)
    {
        std::string key_value = "id"+std::to_string(i);
        std::string label_value;
        for(std::size_t j = 0;j<labels.size();++j)
        {
            label_value += labels[j]+"_"+std::to_string(i)+",";
        }
        label_value.pop_back();
        batch_writer.AddItem(key_value,label_value);
    }
    EXPECT_EQ(batch_writer.TotalCount(),kMaxItemSize);
    batch_writer.Release();
    EXPECT_EQ(batch_writer.TotalCount(),num);
    std::unique_ptr<InputStream> in(new FileInputStream(file_path));

    ReaderOptions r_ops;
    Schema s;
    s.feature_types.insert(s.feature_types.end(),fileds.size()+labels.size(),Schema::STRING);
    // s.feature_types = {Schema::STRING,Schema::FLOAT,Schema::DOUBLE};
    s.feature_names.insert(s.feature_names.end(),fileds.begin(),fileds.end());
    s.feature_names.insert(s.feature_names.end(),labels.begin(),labels.end());

    r_ops.file_schema = s;
    r_ops.batch_size = num;
    r_ops.use_header_order = true;


    CsvReader reader(r_ops, std::move(in));
    reader.Init();
    std::size_t i = 0;
    EXPECT_EQ(fileds.size()+labels.size(),reader.Headers().size());
    for(const std::string& str : fileds)
    {
        EXPECT_EQ(str,reader.Headers()[i++]);
    }
    for(const std::string& str : labels)
    {
        EXPECT_EQ(str,reader.Headers()[i++]);
    }
    ColumnVectorBatch batch;

    EXPECT_EQ(reader.Next(&batch), true);
    EXPECT_EQ(batch.Shape().cols, fileds.size()+labels.size());
    EXPECT_EQ(batch.Shape().rows, num);
    for(std::size_t i = 0;i<num;++i)
    {
        EXPECT_EQ(batch.At<std::string>(i, 0), "id"+std::to_string(i));
        for(std::size_t j = 0;j<labels.size();++j)
        {
            EXPECT_EQ(batch.At<std::string>(i, j+1), labels[j]+"_"+std::to_string(i));
        }
    }
    EXPECT_EQ(std::remove(file_path.c_str()),0);
}
/*
bazel test //src/kernel/data_db:batch_writer_test --test_filter=BatchWriterTest.client_filter
*/
TEST_F(BatchWriterTest, client_filter)
{
    // cout<<"da1"<<endl;
    std::string file_path("haha.csv");
    std::vector<std::string> fileds{"id"};
    std::vector<std::string> labels{"label1","label2","label3"};
    BatchWriter batch_writer(file_path,fileds,labels);
    std::size_t num = 5000;
    for(std::size_t i = 0;i<num;++i)
    {
        std::string key_value = "id"+std::to_string(i);
        std::string label_value;
        for(std::size_t j = 0;j<labels.size();++j)
        {
            label_value += labels[j]+"_"+std::to_string(i)+",";
        }
        label_value.pop_back();
        batch_writer.AddItem(key_value,label_value);
    }
    EXPECT_EQ(batch_writer.TotalCount(),kMaxItemSize);
    batch_writer.Release();
    EXPECT_EQ(batch_writer.TotalCount(),num);
    // cout<<"da2"<<endl;

    std::vector<std::string> client_labels{"label2"};
    std::shared_ptr<spu::psi::IBatchProvider> batch_provider = std::make_shared<spu::psi::CsvBatchProvider>(file_path,fileds, labels);
    // cout<<"da3"<<endl;

    std::string file_path2("haha2.csv");

    BatchWriter batch_writer2(file_path2,fileds,client_labels);
    batch_writer2.EnableClientFilter(labels);
    // cout<<"da4"<<endl;
    while(true)
    {
        // cout<<"da555"<<endl;
        std::vector<std::string> filed_result;
        std::vector<std::string> label_result;
        std::tie(filed_result,label_result) =  batch_provider->ReadNextBatchWithLabel(4096);
        if(filed_result.empty())
            break;
        for(std::size_t i=0;i<filed_result.size();++i)  
        {
            // cout<<"da5 i="<<i<<", filed_result="<<filed_result[i]<<", label_result="<<label_result[i]<<endl;
            batch_writer2.AddItem(filed_result[i],label_result[i]);
        }
    }
    // cout<<"da3"<<endl;
    batch_writer2.Release();
    std::ifstream inputFile(file_path2);
    EXPECT_EQ(inputFile.is_open(),true);
    std::string line;
    std::getline(inputFile, line);
    std::size_t j=0;
    while(std::getline(inputFile, line))
    {
        std::vector<absl::string_view> result = absl::StrSplit(line, ',');
        EXPECT_EQ(result.size(),2);
        std::string key_value = "id"+std::to_string(j);
        EXPECT_EQ(result[0],key_value);
        std::string label_value = client_labels[0]+"_"+std::to_string(j);
        EXPECT_EQ(result[1],label_value);
        ++j;
    }
    // cout<<"da4"<<endl;
    EXPECT_EQ(j,num);
}