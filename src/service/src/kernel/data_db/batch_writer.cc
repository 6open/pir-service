// Copyright 2022 Ant Group Co., Ltd.
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

#include "batch_writer.h"

#include <map>
#include <iostream>

BatchWriter::BatchWriter(const std::string& file_path,const std::vector<std::string>& fileds,const std::vector<std::string>& labels,
                     char field_delimiter, char line_delimiter)
:file_path_(file_path)
,fileds_(fileds)
,labels_(labels)
,field_delimiter_(1, field_delimiter)
,line_delimiter_(1, line_delimiter)
,item_key_(4096)
,item_label_(4096)
{
   out_.open(file_path_, std::ios::trunc);
   WriteHeader();
}

BatchWriter::~BatchWriter()
{
    Release();
}

void BatchWriter::Release()
{
    if(out_.is_open())
    {
        Flush();
        out_.close();
    }
}

void BatchWriter::WriteHeader()
{
    std::string headstr;
    for(const std::string& str : fileds_)
        headstr += str + field_delimiter_;
    for(const std::string& str : labels_)
        headstr += str + field_delimiter_;
    headstr.pop_back();
    out_ << headstr << line_delimiter_;
}

void BatchWriter::EnableClientFilter(const std::vector<std::string>& server_labels)
{
    enable_client_filter_ = true;
    std::map<std::string,int> maps;
    for(std::size_t i=0;i<server_labels.size();++i)
    {
        maps[server_labels[i]] = i;
    }
    label_index_.clear();
    for(std::size_t i=0;i<labels_.size();++i)
    {
        // auto iter = 
        label_index_.push_back(maps[labels_[i]]);
    }
    // split_item_label_.resize(label_index_.size());
    // for(std::size_t i=0;i<split_item_label_.size();++i)
    // {
    //     split_item_label_[i].resize(4096);
    // }
}

void BatchWriter::AddItem(const std::string& key_values,const std::string& label_values)
{
    if(enable_client_filter_ ) {
        std::vector<absl::string_view> result = absl::StrSplit(label_values, ',');
        out_ << key_values;
        for(std::size_t i=0;i<label_index_.size();++i)
        {
            out_<<field_delimiter_<<result[label_index_[i]];
        }
        out_<<line_delimiter_;
    } else {
        item_key_[vec_count_] = key_values;
        item_label_[vec_count_] = label_values;
    }
    vec_count_++;
    if(vec_count_ >= kMaxItemSize)
    {
        Flush();
    }
}

void BatchWriter::Flush()
{

    if(enable_client_filter_ ) {
        // for(std::size_t i = 0;i<vec_count_;++i)
        // {
        //     out_ << item_key_[i];
        //     for(std::size_t j=0;j<label_index_.size();++j)
        //     {
        //         std::cout<<"BatchWriter::Flush i="<<i<<", j="<<j<<", str="<<split_item_label_[i][j]std::endl;
        //         out_<<field_delimiter_<<split_item_label_[i][j];
        //     }
        //     out_<<line_delimiter_;
        // }
    } else {
        for(std::size_t i = 0;i<vec_count_;++i)
        {
            out_ << item_key_[i]<<field_delimiter_<<item_label_[i]<<line_delimiter_;
        }
    }
    total_count_ += vec_count_;
    vec_count_ = 0;
}
