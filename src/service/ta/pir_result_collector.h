#pragma once

#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "src/http_server/proto/http.pb.h"
#include "src/kernel/data_db/time_profiler.pb.h"

using namespace mpc::pir;
using namespace example;
using namespace std;
class PirResultCollector
{
public:
    PirResultCollector()
    {

    }
    void SetInfo(const std::string& file,uint32_t server_num,uint32_t client_num,uint32_t thread_num)
    {
        filename_ = file;
        out_.open(filename_);
        if(!out_.is_open())
            cout<<"file open error :"<<filename_<<endl;
        out_<<"query server num= "<<server_num<<", client_num= "<<client_num<<", thread_num="<<thread_num<<endl;
    }

    ~PirResultCollector()
    {
        cout<<"PirResultCollector file: "<<filename_<<endl;
        Flush();
        if(out_.is_open())
            out_.close();
    }
    void OnSetupCallback(uint32_t spend)
    {
        out_<<"setup: "<<spend<<endl;
    }
    void OnClientCallback(const PirCallbackRequest* request)
    {
        client_query_time.push_back(request->spend());
        server_profiles_.push_back(request->server_profile());
        client_profiles_.push_back(request->client_profile());
    }
    void Flush()
    {
        out_<<"query number count " <<client_query_time.size()<<endl;
        out_<<"quer time spend"<<endl;
        out_<<"[";
        uint32_t sum = 0;
        uint32_t avg = 0;
        for(uint32_t i : client_query_time){
            out_<<i<<",";
            sum += i;
        }
        out_<<"]"<<endl;
        if(client_query_time.size()!=0) {
            avg = sum/client_query_time.size();
        }
        out_<<"avg: "<<avg<<endl;
        for(std::size_t i=0;i<server_profiles_.size();++i)
        {
            TimeProfilerInfo server_info;
            server_info.ParseFromString(server_profiles_[i]);
            std::map<std::string,uint32_t> smaps;
            for (const auto& key_value_pair : server_info.item()) {
                smaps[key_value_pair.key()] = key_value_pair.value();
            }

            TimeProfilerInfo client_info;
            client_info.ParseFromString(client_profiles_[i]);
            std::map<std::string,uint32_t> cmaps;
            for (const auto& key_value_pair : client_info.item()) {
                cmaps[key_value_pair.key()] = key_value_pair.value();
            }
            out_<<"smaps: [";
            for(auto& item : smaps)
                out_<<item.first<<": "<<item.second<<" ";
            out_<<"]"<<endl;
            out_<<"cmaps: [";
            for(auto& item : cmaps)
                out_<<item.first<<": "<<item.second<<" ";
            out_<<"]"<<endl;
            cmaps["NETWORK"]  =  cmaps["NETWORK"] - smaps["LOAD_DB"]-smaps["ALGO"];
            smaps["NETWORK"] = smaps["NETWORK"]-cmaps["ALGO"] ;

            out_<<"smaps: [";
            for(auto& item : smaps)
                out_<<item.first<<": "<<item.second<<" ";
            out_<<"]"<<endl;
            out_<<"cmaps: [";
            for(auto& item : cmaps)
                out_<<item.first<<": "<<item.second<<" ";
            out_<<"]"<<endl;

            for(auto& item : cmaps)
            {
                item.second += smaps[item.first];
                smaps.erase(item.first);
            }
            for(auto& item : smaps)
            {
                cmaps[item.first] += item.second;
            }
            out_<<"smaps: [";
            for(auto& item : smaps)
                out_<<item.first<<": "<<item.second<<" ";
            out_<<"]"<<endl;
            uint32_t sum = 0;
            out_<<"cmaps: [";
            for(auto& item : cmaps) {
                out_<<item.first<<": "<<item.second<<" ";
                sum += item.second;
            }
            out_<<" sum: "<<sum;
            out_<<"]"<<endl;
        }

    }
private:
    std::string filename_;
    std::ofstream out_;
    std::vector<uint32_t> client_query_time;
    std::vector<std::string> server_profiles_;
    std::vector<std::string> client_profiles_;

};