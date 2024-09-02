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

#include "time_profiler.h"

TimeProfiler::~TimeProfiler()
{
    Print();
}
void TimeProfiler::Count(const std::string& profile_name,ProType pro_type)
{
    if(disabled_ == true)
        return;
    if(!last_func.empty()){
        std::chrono::duration<double, std::milli> duration = std::chrono::high_resolution_clock::now() - start_time;
        double time_use = duration.count();
        auto iter = maps.find(last_func);
        if(iter == maps.end())
            maps.insert({last_func,TimeTrack(last_pro_type)});
        maps[last_func].Count(time_use);
    }
    last_func = profile_name;
    last_pro_type = pro_type;
    start_time = std::chrono::high_resolution_clock::now();
}

void TimeProfiler::Flush()
{
    if(!last_func.empty()){
        std::chrono::duration<double, std::milli> duration = std::chrono::high_resolution_clock::now() - start_time;
        double time_use = duration.count();
        auto iter = maps.find(last_func);
        if(iter == maps.end())
            maps.insert({last_func,TimeTrack(last_pro_type)});
        maps[last_func].Count(time_use);
    }
    last_func.clear();
}

void TimeProfiler::Print()
{
    for(auto& item : maps)
    {
        std::cout<<item.first<<", "<<item.second<<std::endl;
    }
}

std::string TimeProfiler::ProtoString()
{
    TimeProfilerInfo info;
    std::map<std::string,uint32_t> time_map;
    for(auto& item : maps)
    {
        time_map[to_string(item.second.Type())] += item.second.Sum();
        // std::cout<<item.first<<", "<<item.second<<std::endl;
    }
    for(auto& item : time_map)
    {
        Item* it = info.add_item();
        it->set_key(item.first);
        it->set_value(item.second);
    }
    return info.SerializeAsString();
}