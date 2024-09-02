#pragma once

#include<iostream>
#include <fstream>
#include<vector>
#include <random>
#include <filesystem>
#include <unordered_set>
#include "absl/strings/escaping.h"
#include "fmt/format.h"
#include "yacl/crypto/utils/rand.h"
using namespace std;
class DataGenerator
{
public:
    DataGenerator(uint32_t server_num,uint32_t client_num,uint32_t query_num,double query_rate,const std::string& outdir,bool use_cache = false,std::vector<std::string> label_columns = {"label1","label2"})
    {
        query_rate_ = query_rate;
        server_num_ = server_num;
        client_num_ = client_num;
        query_num_ = query_num;
        outdir_  = outdir;
        isUserCache = use_cache;
        label_columns_ = label_columns;
        cout<<"DataGenerator server_num="<<server_num_<<", client_num="<<client_num_<<",query_num_="<<query_num_<<", query_rate="<<query_rate_<<", outdir_="<<outdir_<<", use_cache="<<isUserCache<<endl;

        CreateDataName();
        CreateOrReadFile();
    }
    void CreateDataName()
    {
        server_name_ = "pir_server_data_"+std::to_string(server_num_)+".csv";
        client_name_.push_back("pir_client_data_"+std::to_string(server_num_)+"_"+std::to_string(client_num_)+".csv");
        // for(std::size_t i=0;i<client_num_;++i)
        // {
        //     client_name_.push_back("pir_client_data_"+std::to_string(server_num_)+"_"+std::to_string(query_rate_)+"_"+std::to_string(i)+".csv");
        // }
    }
    bool NeedCreate()
    {
        std::string s_path = outdir_+"/"+server_name_;
        if(isUserCache && (std::filesystem::exists(s_path)))
            return false;
        else
            return true;
    }
    void CreateOrReadFile()
    {
        if(NeedCreate())
        {
            CreateFile();
        }
        else
        {
            ReadServer();
        }
    }
    void CreateFile()
    {
        cout<<"0 server create succeed server_num="<<server_num_<<", client_num="<<client_num_<<", query_rate="<<query_rate_<<endl;
        // for(std::size_t i=0;i<client_num_;++i)
        // {
        //     client_name_.push_back("pir_client_data_"+std::to_string(server_num_)+"_"+std::to_string(query_rate_)+"_"+std::to_string(i)+".csv");
        // }
        std::ofstream server_stream;
        std::string s_path = outdir_+"/"+server_name_;
        server_stream.open(s_path);
        if(!server_stream.is_open())
        {
            cout<<"open server file failed:"<<s_path<<endl;
            return;
        }
        server_stream << "id";
        for(std::size_t i =0;i<label_columns_.size();++i)
            server_stream<<","<<label_columns_[i];
        server_stream <<  std::endl;
        std::size_t label_size = 8;
        ids.resize(server_num_);
        for (size_t idx = 0; idx < server_num_; idx++) 
        {
            std::string a_item = fmt::format("{:012d}", idx);
            // std::string a_item = std::to_string(idx);
            // cout<<"server appened a_item="<<a_item<<", idx="<<idx<<", query_rate="<<query_rate<<endl;

            std::vector<uint8_t> label_bytes = yacl::crypto::RandBytes(label_size);
            // cout<<"gen data:"<<absl::string_view(reinterpret_cast<char *>(label_bytes.data()),label_bytes.size())<<", size="<<label_bytes.size()<<endl;
            server_stream << a_item;
            for(std::size_t i =0;i<label_columns_.size();++i)
                server_stream << "," << absl::BytesToHexString(absl::string_view(reinterpret_cast<char *>(label_bytes.data()),label_bytes.size()));
            server_stream << std::endl;
            ids_hash.insert(a_item);
            ids[idx] = (a_item);
        }
        server_stream.close();
        cout<<"server create succeed server_num="<<server_num_<<", client_num="<<client_num_<<", query_rate="<<query_rate_<<endl;
        for(std::size_t i=0;i<client_name_.size();++i)
        {
            std::ofstream client_stream;
            std::string c_path = outdir_+"/"+client_name_[i];
            client_stream.open(c_path);
            client_stream << "id" << std::endl;
            std::mt19937 rand;
            std::bernoulli_distribution dist1(query_rate_);
            uint32_t count = 0;
            for(std::size_t j=0;j<ids.size();++j)
            {
                if (dist1(rand)) {
                    ++count;
                    client_stream << ids[j] << std::endl;
                }
            }
            cout<<"DataGenerator count="<<count<<", query_num_="<<query_num_<<endl;
            for(std::size_t i=0;count<query_num_;++i)
            {
                count++;
                client_stream << ids[i] << std::endl;
            }
            client_stream.close();
            cout<<"client create succeed i="<<i<<", client_num="<<client_num_<<", query_rate="<<query_rate_<<endl;
        }
    }
    void ReadServer()
    {
        std::string s_path = outdir_+"/"+server_name_;
        std::ifstream fileStreamA(s_path);
        cout<<"0 ReadServer s_path="<<s_path<<endl;
        std::string lineA;
        std::getline(fileStreamA, lineA);  
        if (fileStreamA.is_open()) {
            while (std::getline(fileStreamA, lineA)) {
                // 假设 id 列位于第一列，并使用逗号作为分隔符
                size_t pos = lineA.find(',');
                if (pos != std::string::npos) {
                    std::string id = lineA.substr(0, pos);
                    std::string la = lineA.substr(pos+1);
                    // cout<<"read id="<<id<<", la="<<la<<endl;
                    ids_hash.insert(id);
                }
            }
            cout<<"ReadServer succeed  s_path="<<s_path<<endl;
            fileStreamA.close();
        } else {
            std::cerr << "can not open:" << s_path << std::endl;
            return;
        }
    }
    bool CheckFile(const std::vector<std::string>& result, std::vector<std::string>& in_id)
    {

        for(auto id:result){
            std::cout << id << std::endl;
            if (ids_hash.find(id) == ids_hash.end()) {
                return false;
            }
        }
        return true;
    }
public:
    uint32_t server_num_;
    uint32_t client_num_;
    uint32_t query_num_;
    // std::vector<uint32_t> client_num_;
    std::string server_name_;
    std::string outdir_;
    std::vector<std::string> client_name_;
    double query_rate_;
    std::vector<std::string> ids;
    std::unordered_set<std::string> ids_hash;
    bool isUserCache = false;
    std::vector<std::string> label_columns_;
};

