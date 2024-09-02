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
#include "time_statistics.h"
#include <time.h>
#include <iostream>
#include "spdlog/spdlog.h"

using namespace std;
namespace mpc{
namespace utils{

TimerStatistics::TimerStatistics(const char* funcName)
{
    _funcname = funcName;
    _begin = std::clock();
}

TimerStatistics::~TimerStatistics()
{
    _end = std::clock();
    double time_in_milliseconds = (_end - _begin) * 1000.0 / CLOCKS_PER_SEC;
    //cout<<"Function name:"<<_funcname<<" Elapsed : "<<time_in_milliseconds<<endl;
    SPDLOG_INFO("{} Elapsed {}",_funcname,time_in_milliseconds);
}  

} // end of namespace utils
} // end of namespace mpc