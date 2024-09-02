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


#include "searcher.h"

#include <openssl/md5.h>
#include <iomanip>
#include <sstream>
DataOwner::DataOwner(const std::string& name)
:name_(name)
{

}
bool DataOwner::init(const std::string& filename,const std::string& col_name,std::vector<std::string>& feature,std::string& errorMessage)
{
    try {
    std::unique_ptr<InputStream> in(new FileInputStream(filename));
    Schema s;
    s.feature_types = {Schema::STRING};
    feature_.push_back(col_name);
    if(!feature.empty())
    {
        std::vector<Schema::Type> tp(feature.size(),Schema::STRING);
        feature_.insert(feature_.end(),feature.begin(),feature.end());
        s.feature_types.insert(s.feature_types.end(),tp.begin(),tp.end());
    }
    s.feature_names = feature_;
    feature_size = s.feature_names.size()-1;
    ReaderOptions r_ops;
    //r_ops.column_reader = true;
    r_ops.file_schema = s;
    r_ops.batch_size = s.feature_names.size();
    r_ops.use_header_order = true;
    r_ops.column_reader = false;

    CsvReader reader(r_ops, std::move(in));
    reader.Init();
    if(reader.Next(r_ops.batch_size,&batch_out_) == false)
    {
        SPDLOG_INFO("{} can not get batch",col_name);
        return false;
    }
    SPDLOG_INFO("{} row={} col={}",col_name,batch_out_.Shape().rows,batch_out_.Shape().cols);
    data_.rehash(batch_out_.Shape().rows*1.5);
    for(std::size_t i=0;i<batch_out_.Shape().rows;++i)
    {
        data_.insert({md5(batch_out_.At<std::string>(i, 0)),i});
    }

    //reader.close();
    SPDLOG_INFO("{} load complete",col_name);

    }
    catch(const std::exception& e)
    {
        std::cout<<"begin search "<< e.what()<<std::endl;
        errorMessage = e.what();
        return false;
    }
    return true;
}
std::vector<std::string> DataOwner::exit(const std::string& key)
{
    std::vector<std::string> result;
    auto iter = data_.find(key);
    if(iter != data_.end())
    {
        result.push_back(key);
        int row =iter->second;
        for(auto i=1;i<=feature_size;++i)
            result.push_back(batch_out_.At<std::string>(row, i));
    }
    return result;
}
std::vector<std::vector<std::string>> DataOwner::exit(const std::vector<std::string>& keys,std::vector<std::string>& feature)
{
    std::vector<std::vector<std::string>> ret;
    feature = feature_;
    for(std::size_t i=0;i<keys.size();++i)
    {
        auto subret = exit(keys[i]);
        if(!subret.empty())
            ret.emplace_back(subret);
    }
    return ret;
}

std::string DataOwner::md5(const std::string& str)
{
    unsigned char digest[MD5_DIGEST_LENGTH];
    std::string hello = "hello"+str+"world";
    MD5(reinterpret_cast<const unsigned char*>(hello.c_str()), hello.size(), digest);

    std::stringstream ss;
    for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}