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
#include "gtest/gtest.h"
#include "src/http_server/proto/http.pb.h"
#include "brpc/server.h"
#include "spdlog/spdlog.h"
#include "brpc/channel.h"
#include "bthread/bthread.h"
#include "bthread/condition_variable.h"

#include "src/config/config.h"
#include "src/rpc/rpc_transport/rpc_server.h"
#include <map>
#include <functional>
#include <sys/sysinfo.h>
#include "pir_data_generator.h"
#include "pir_result_collector.h"
// using namespace mpc::rpc;
using namespace mpc::pir;
using namespace example;
using SetupCallback = std::function<void(const std::string& task_id,const std::string& key,uint32_t spend)>;
using ClientCallback = std::function<void(const PirCallbackRequest* request,PirCallbackResponse* response)>;
class QueueServiceImpl :  public example::QueueService {
public:
    QueueServiceImpl(const SetupCallback& setup_callback,const ClientCallback& client_callback)
    :QueueService()
    ,setup_callback_(setup_callback)
    ,client_callback_(client_callback)
    {

    }
    virtual ~QueueServiceImpl() {}
    void start(google::protobuf::RpcController* cntl_base,
               const HttpRequest*,
               HttpResponse*,
               google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);
        brpc::Controller* cntl =
            static_cast<brpc::Controller*>(cntl_base);
        cntl->response_attachment().append("hello world");
    }

    void getstats(google::protobuf::RpcController* cntl_base,
                  const HttpRequest*,
                  HttpResponse*,
                  google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);
        brpc::Controller* cntl =
            static_cast<brpc::Controller*>(cntl_base);
        const std::string& unresolved_path = cntl->http_request().unresolved_path();
        if (unresolved_path.empty()) {
            cntl->response_attachment().append("Require a name after /stats");
        } else {
            cntl->response_attachment().append("Get stats: ");
            cntl->response_attachment().append(unresolved_path);
        }
    }
    void pir_setup_callback(::google::protobuf::RpcController* cntl_base,
              const PirSetupCallbackRequest* request,
              PirSetupCallbackResponse* response,
              ::google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);
        SPDLOG_INFO("pir_setup_callback {}",request->DebugString());
        setup_callback_(request->task_id(),request->data_result().key(),request->spend());
        response->set_code(0);
        response->set_spend(100);
    }

    void pir_client_callback(::google::protobuf::RpcController* cntl_base,
              const PirCallbackRequest* request,
              PirCallbackResponse* response,
              ::google::protobuf::Closure* done) {
        brpc::ClosureGuard done_guard(done);
        SPDLOG_INFO("pir_client_callback {}",request->DebugString());
        client_callback_(request,response);
        response->set_code(0);
        response->set_spend(100);
    }
public:
    // std::map<std::string,std::string> map_keys_;
    SetupCallback setup_callback_;
    ClientCallback client_callback_;
};

class PirMultiQueryTest : public ::testing::Test 
{
public:
    void SetUp() override
    {
    }

    void init(std::size_t t_num,uint32_t s_num,uint32_t query_num,double q = 0.001,bool use_cached = false)
    {
        thread_num = t_num;
        server_num = s_num;
        client_num = thread_num;
        query_rate = q;
        std::cout<<"PirMultiQueryTest::init query_num="<<query_num<<std::endl;
        readys.resize(thread_num,false);
        result_check.resize(thread_num,false);

        // std::vector<std::string> c_data;
        // c_data.push_back("pir_client_data_100000_0.000010_0.csv");
        // SetData("pir_server_data_100000.csv", c_data);
        data_generator = std::make_unique<DataGenerator>(server_num,client_num,query_num,query_rate,outdir,use_cached);
        SetData(data_generator->server_name_,data_generator->client_name_);
    }

    void RunTest(std::size_t thread_num,uint32_t data_num,uint32_t query_num,bool use_cached = false)
    {
        double query_rate = (double)query_num/data_num; 
        std::cout<<"PirMultiQueryTest::RunTest data_num="<<data_num<<", query_num="<<query_num<<" query_rate="<<query_rate<<std::endl;
        init(thread_num,data_num,query_num,query_rate,use_cached);
        // std::string pir_info = outdir+"/pir_info_servernum"+std::to_string(data_num)+"_clientnum"+std::to_string(query_num)+"_threadnum_"+std::to_string(thread_num);
        // collertor.SetInfo(pir_info,data_num,query_num,thread_num);

        // SetData(data_generator->server_name_,data_generator->client_name_);
        auto sc = [this](const std::string& task_id,const std::string& key,uint32_t spend)
            {
                SetpCallback(task_id,key,spend);
            };
        auto cc = [this](const PirCallbackRequest* request,PirCallbackResponse* response)
            {
                ClientCallback(request,response);
            };
        QueueServiceImpl queue_svc(sc,cc);

        if (callback_server.AddService(&queue_svc,
                            brpc::SERVER_DOESNT_OWN_SERVICE,
                            "/v1/queue/start   => start,"
                            "/v1/service/pir/setup_callback => pir_setup_callback,"
                            "/v1/service/pir/client_callback => pir_client_callback,"
                            "/v1/queue/stats/* => getstats") != 0) {
            LOG(ERROR) << "Fail to add queue_svc";
            return;
        }
        brpc::ServerOptions server_options;
        callback_port = 13000;
        server_options.idle_timeout_sec = -1;
        if (callback_server.Start(callback_port, &server_options) != 0) {
            LOG(ERROR) << "Fail to start HttpServer";
            return ;
        }
        // server.RunUntilAskedToQuit();
        SendSetupRequest();
        std::unique_lock<bthread::Mutex> lock(msg_mutex_);

        uint32_t wait_count =0;
        uint32_t max_wait_count =10;
        while (!isAllReady()) {
        if (msg_db_cond_.wait_for(lock, 60*10*1000 * 1000) == ETIMEDOUT) {
            wait_count++;
            std::cout<<"Get data timeout wait_count="<<wait_count<<std::endl;
            if(wait_count > max_wait_count)
            {
                break;
            }
            // break;
        }
        }
        // for(std::size_t i=0;i< result_check.size();++i )
        //         EXPECT_TRUE( result_check[i]);
    }

    void SetpCallback(const std::string& task_id,const std::string& key,uint32_t spend)
    {

        collertor.OnSetupCallback(spend);
        std::vector<std::thread> sthread;
        std::vector<std::thread> cthread;
        auto sfun = [&](std::size_t i)
        {
            std::string new_task = task_id+"_"+std::to_string(i);
            SendServerRequest(new_task,key);
        };
        auto cfun = [&](std::size_t i)
        {
            std::string new_task = task_id+"_"+std::to_string(i);
            maps[new_task] = i;
            SendClientRequest(new_task,key,i);
        };
        for(std::size_t i=0;i<thread_num;++i)
        {
            sthread.emplace_back(std::thread(sfun,i));
            cthread.emplace_back(std::thread(cfun,i));
        }
        for(std::size_t i=0;i<thread_num;++i)
        {
            sthread[i].join();
            cthread[i].join();
        }
    }

    void ClientCallback(const PirCallbackRequest* request,PirCallbackResponse* response)
    {
        std::unique_lock<bthread::Mutex> lock(msg_mutex_);
        collertor.OnClientCallback(request);
        EXPECT_EQ(request->status(),200);
        std::size_t num = maps[request->task_id()];
        readys[num] = true;
        // ResultCheck(num,request->data_result().dataset_name());
        // request->data_result().data_content();

        // std::vector<std::string> result;
        // for (int i = 0; i < request->data_result_size(); ++i) {
        //     const PirDataContent& dataContent = request->data_result(i);
        //     std::cout << "PirDataContent " << i << ":" << std::endl;
        //     result.push_back(dataContent.first());
        // }
        // ResultCheck(num, result);
        // if(isAllReady())
            msg_db_cond_.notify_all();
    }

    void ResultCheck(std::size_t num,const std::vector<std::string>& result)
    {
        std::vector<std::string> in_id;
        result_check[num] = data_generator->CheckFile(result, in_id);
        cout << "in_id size : " << in_id.size();
        // cout<<"check result:num="<<num<<", path="<<path<<", ret="<<result_check[num]<<endl;
    }

    bool isAllReady()
    {
        for(std::size_t i=0;i<readys.size();++i)
        {
            if(readys[i]==false)
                return false;
        }
        return true;
    }

    void SendServerRequest(const std::string& task_id,const std::string& key)
    {
        brpc::Channel channel;
        brpc::ChannelOptions options;
        //options.max_concurrency = 10000;
        options.timeout_ms = 120000;  
        options.protocol = brpc::PROTOCOL_HTTP;
        std::string setupurl = CreateServerRequestAddr()+"/v1/service/pir/server";
        if (channel.Init(setupurl.c_str(), &options) != 0)
        {
            std::cout<<("Fail to initialize channel")<<std::endl;
        }
        brpc::Controller cntl;
        cntl.http_request().uri() = setupurl;  // 设置为待访问的URL
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        PirServerRequest request;
        PirServerResponse response;
        std::string algo = "SE";
        request.set_version("1");
        request.set_task_id(task_id);
        request.set_key(key);
        request.set_algorithm(algo);
        request.set_rank(0);
        request.set_testlocal(true);
        request.add_members(member_self);
        request.add_members(member_peer);
        request.add_ips(ip1);
        request.add_ips(ip2);

        channel.CallMethod(NULL, &cntl, &request, &response, NULL/*done*/);
        SPDLOG_INFO("setup respone :{}",response.DebugString());
        // SPDLOG_INFO("setup failed ,remove data handeler taskid:{} error{} code:{} message:{}",task_id,cntl.ErrorText(),response.status(),response.message());
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(response.status(),200);
    }
    string ReadClientFile(const std::string fileName)
    {
        std::ifstream file(fileName);
        std::vector<std::string> lines;
        std::string line;

        std::ostringstream oss;
        if (file.is_open()) {
            std::getline(file, line);
            while (std::getline(file, line)) { 
                lines.push_back(line); 
                oss << line;
                oss << ",";
            }
            file.close(); 
        } else {
            std::cerr << "无法打开文件" << std::endl;
        }

        for (const auto& item : lines) {
            oss << item;
            oss << ",";
        }

        return oss.str();
    }
    void SendClientRequest(const std::string& task_id,const std::string& key,std::size_t i)
    {
        brpc::Channel channel;
        brpc::ChannelOptions options;
        //options.max_concurrency = 10000;
        options.timeout_ms = 120000;  
        options.protocol = brpc::PROTOCOL_HTTP;
        std::string setupurl = CreateClientRequestAddr()+"/v1/service/pir/client";
        if (channel.Init(setupurl.c_str(), &options) != 0)
        {
            std::cout<<("Fail to initialize channel")<<std::endl;
        }
        brpc::Controller cntl;
        cntl.http_request().uri() = setupurl;  // 设置为待访问的URL
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        PirClientRequest request;
        PirClientResponse response;
        std::string algo = "SE";
        std::string data =  client_data[i];

        request.set_version("1");
        request.set_task_id(task_id);
        request.set_key(key);
        request.set_algorithm(algo);
        request.set_rank(1);
        request.set_testlocal(true);
        request.add_members(member_self);
        request.add_members(member_peer);
        request.add_ips(ip1);
        request.add_ips(ip2);
        request.add_fields("id");
        request.add_labels("label1");
        request.add_labels("label2");
        request.set_data_file(data);
        
        SPDLOG_INFO("client_data {}", outdir + "/" + data);
        auto client_data = ReadClientFile(outdir + "/" + data);

        request.set_query_ids(client_data);
        std::string callback_url = CreateCallbackAddr()+"/v1/service/pir/client_callback";
        request.set_callback_url(callback_url);
        SPDLOG_INFO("send client request {}",request.DebugString());
        channel.CallMethod(NULL, &cntl, &request, &response, NULL/*done*/);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(response.status(),200);
    }
    void SendSetupRequest()
    {
        brpc::Channel channel;
        brpc::ChannelOptions options;
        //options.max_concurrency = 10000;
        options.timeout_ms = 120000;  
        options.protocol = brpc::PROTOCOL_HTTP;
        std::string setupurl = CreateServerRequestAddr() + "/v1/service/pir/setup";
        if (channel.Init(setupurl.c_str(), &options) != 0)
        {
            std::cout<<("Fail to initialize channel")<<std::endl;
        }
        brpc::Controller cntl;
        cntl.http_request().uri() = setupurl;  // 设置为待访问的URL
        cntl.http_request().set_method(brpc::HTTP_METHOD_POST);
        PirSetupRequest request;
        // request.
        PirSetupResponse response;

        std::string taskid = "1100";
        std::string algo = "SE";
        std::string data =  server_data;
        std::string callback_url = CreateCallbackAddr()+"/v1/service/pir/setup_callback";
        request.set_version("1");
        request.set_task_id(taskid);
        request.set_algorithm(algo);
        request.set_callback_url(callback_url);
        request.set_data_file(data);
        request.add_fields("id");
        request.add_labels("label1");
        request.add_labels("label2");
        channel.CallMethod(NULL, &cntl, &request, &response, NULL/*done*/);
        EXPECT_FALSE(cntl.Failed());
        EXPECT_EQ(response.status(),200);
    }
    void SetData(const std::string& sdata,const std::vector<std::string>& cdata)
    {
        server_data = sdata;
        client_data = cdata;
    }
    void SetThreadNum(std::size_t num)
    {
        thread_num = num;
        readys.resize(thread_num,false);
    }
    void SetServerClinet(const std::string& sp,const std::string& cp,uint32_t sport,uint32_t cport)
    {
        ip1 = sp;
        ip2 = cp;
        port1 = sport;
        port2 = cport;
    }
    std::string CreateServerRequestAddr()
    {
        return "http://" + ip1 +":"+ std::to_string(port1);  
    }
    std::string CreateClientRequestAddr()
    {
        return "http://" + ip2 +":"+ std::to_string(port2);  
    }
    void SetCallbackInfo(const std::string& ip,uint32_t listen_port,uint32_t mapping_port)
    {
        local_ip = ip;
        callback_port = listen_port;
        mapping_callback_port = mapping_port;
    }
    std::string CreateCallbackAddr()
    {
        return "http://" + local_ip + ":" + std::to_string(mapping_callback_port);
    }
public:
    std::string member_self = "member_self";
    std::string member_peer = "member_peer";
    std::string ip1 = "127.0.0.1";
    std::string ip2 = "127.0.0.1";
    uint32_t port1 = 12601;
    uint32_t port2 = 12601;
    std::string local_ip = "127.0.0.1";

    //used for local listen
    uint32_t callback_port = 13000;
    //for remote call
    //docker bind : mapping_callback_port -> callback_port
    uint32_t mapping_callback_port = 13000;
    bthread::Mutex msg_mutex_;
    bthread::ConditionVariable msg_db_cond_;
    // bool ready = false;
    std::size_t thread_num = 10;
    std::vector<bool> readys;
    std::vector<bool> result_check;
    std::map<std::string,std::size_t> maps;
    std::string server_data;
    std::vector<std::string> client_data;
    brpc::Server callback_server;

    uint32_t server_num = 100000;
    uint32_t client_num = thread_num;
    double query_rate = 0.0001;
    std::string outdir = "/data/storage/dataset/pre_deal_result";
    std::unique_ptr<DataGenerator> data_generator;
    std::string defualt_result = "/home/admin/data/result";

    PirResultCollector collertor;
    std::vector<std::string> key_columns_{"id"};
    std::vector<std::string> label_columns_{"label"};
    std::vector<std::string> client_label_columns_{"label"};
};

TEST_F(PirMultiQueryTest, test100_1)
{
    std::cout<<"PirMultiQueryTest::test10w_100"<<std::endl;
    RunTest(1,100,1);
}

TEST_F(PirMultiQueryTest, test1000_1)
{
    std::cout<<"PirMultiQueryTest::test10w_100"<<std::endl;
    RunTest(1,1000,1);
}

TEST_F(PirMultiQueryTest, test1w_1)
{
    std::cout<<"PirMultiQueryTest::test10w_100"<<std::endl;
    RunTest(1,10000,1);
}

TEST_F(PirMultiQueryTest, test100w)
{
    std::cout<<"PirMultiQueryTest::test10w_100"<<std::endl;
    RunTest(1,1000000,10);
}

TEST_F(PirMultiQueryTest, test10w_100)
{
    std::cout<<"PirMultiQueryTest::test10w_100"<<std::endl;
    RunTest(1,100000,100);
}

TEST_F(PirMultiQueryTest, test10w_1000)
{
    std::cout<<"PirMultiQueryTest::test10w_1000"<<std::endl;
    RunTest(1,100000,1000);
}

TEST_F(PirMultiQueryTest, test10w_10000)
{
    std::cout<<"PirMultiQueryTest::test10w_10000"<<std::endl;
    RunTest(1,100000,10000);
}


/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10w_1
*/
TEST_F(PirMultiQueryTest, test10w_1)
{
    std::cout<<"PirMultiQueryTest::test10w_1 "<<std::endl;
    RunTest(1,100000,1);
}
/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10w_10
*/
TEST_F(PirMultiQueryTest, test10w_10)
{
    std::cout<<"PirMultiQueryTest::test10w_10 "<<std::endl;
    RunTest(1,100000,10);
}

TEST_F(PirMultiQueryTest, test100w_1)
{
    std::cout<<"PirMultiQueryTest::test100w_1  query_rate="<<query_rate<<std::endl;
    RunTest(1,1000000,1);   
}
/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test100w_10
*/
TEST_F(PirMultiQueryTest, test100w_10)
{
    std::cout<<"PirMultiQueryTest::test100w_10  query_rate="<<query_rate<<std::endl;
    RunTest(1,1000000,10);   
}
/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test100w_100
*/
TEST_F(PirMultiQueryTest, test100w_100)
{
    std::cout<<"PirMultiQueryTest::test100w_10  query_rate="<<query_rate<<std::endl;
    RunTest(1,1000000,100);   
}
/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test100w_cahce
*/
TEST_F(PirMultiQueryTest, test100w_cahce)
{
    std::cout<<"PirMultiQueryTest::test100w_cahce  query_rate="<<query_rate<<std::endl;
    RunTest(1,1000000,10,true);   
}
/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test1000w_1
*/
TEST_F(PirMultiQueryTest, test1000w_1)
{
    std::cout<<"PirMultiQueryTest::test1000w query_rate="<<query_rate<<std::endl;
    RunTest(1,10000000,1);
}

/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test1000w_10
*/
TEST_F(PirMultiQueryTest, test1000w_10)
{
    std::cout<<"PirMultiQueryTest::test1000w_10 query_rate="<<query_rate<<std::endl;
    RunTest(1,10000000,10);
}

/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test1000w_100
*/
TEST_F(PirMultiQueryTest, test1000w_100)
{
    std::cout<<"PirMultiQueryTest::test1000w_100 query_rate="<<query_rate<<std::endl;
    RunTest(1,10000000,100);
}

TEST_F(PirMultiQueryTest, test1000w_10000)
{
    std::cout<<"PirMultiQueryTest::test1000w_100 query_rate="<<query_rate<<std::endl;
    RunTest(1,10000000,10000);
}

/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10000w_1
*/
TEST_F(PirMultiQueryTest, test10000w_1)
{
    std::cout<<"PirMultiQueryTest::test10000w_1 query_rate="<<query_rate<<std::endl;
    RunTest(1,100000000,1);
}

/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10000w_10
*/
TEST_F(PirMultiQueryTest, test10000w_10)
{
    std::cout<<"PirMultiQueryTest::test10000w_10 query_rate="<<query_rate<<std::endl;
    RunTest(1,100000000,10);
}

/*
bazel test //ta:pir_multi_query --test_filter=PirMultiQueryTest.test10000w_100
*/
TEST_F(PirMultiQueryTest, test10000w_100)
{
    std::cout<<"PirMultiQueryTest::test10000w_100  query_rate="<<query_rate<<std::endl;
    RunTest(1,100000000,100);
}