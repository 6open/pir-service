import threading
import requests
import json
from flask import Flask, request, jsonify
import time
import csv
import json
import concurrent.futures

app = Flask(__name__)

key_value = None
key_lock = threading.Lock()  # 互斥锁

@app.route('/setup_callback', methods=['POST'])
def setup_callback():
    data = request.get_json()  # 获取通过回调URL发送的数据
    global key_value

    # 在这里处理回调数据
    print("Received callback data:")
    print(data)
    
    with key_lock:
        key_value = data.get('data_result', {}).get('key')  # 保护 key_value

    if key_value:
        print(f"Received callback data with key: {key_value}")
        
    # 执行处理操作，生成响应数据
    response_data = {
        "code": 0,
        "spend": 100
    }
    return jsonify(response_data)  # 返回 JSON 响应数据

@app.route('/client_callback', methods=['POST'])
def client_callback():
    data = request.get_json()  # 获取通过回调URL发送的数据
    
    result_value = data.get('data_result')  # 保护 key_value
        
    # 在这里处理回调数据
    print("Received callback data:")
    print(data)
    if result_value:
        print(f"Result data: \n{result_value}")
    # 将CSV字符串解析为字典列表
    # csv_data = result_value.strip()
    # csv_reader = csv.DictReader(csv_data.splitlines())
    # json_data = [row for row in csv_reader]
    # json_string = json.dumps(json_data, indent=4)
    # print(json_string)

    # 执行处理操作，生成响应数据
    response_data = {
        "code": 0,
        "spend": 100
    }
    return jsonify(response_data)  # 返回 JSON 响应数据

def send_setup():
    time.sleep(3)
    data = {
        "task_id": "1111",
        "callback_url": "http://192.168.50.162:5000/setup_callback",
        "data_file": "pir_server_data_100000.csv",
        "fields": ["id"],
        "labels": ["label1", "label2"]
    }

    data_json = json.dumps(data)

    url = "http://192.168.50.162:12641/v1/service/pir/setup"
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, data=data_json, headers=headers)

    if response.status_code == 200:
        callback_data = response.json()
        key_value = callback_data.get('data_result', {}).get('key')

        if key_value:
            print(f"Received callback data with key: {key_value}")
    else:
        print(f"请求失败，状态码: {response.status_code}")
        print(response.text)

def send_server_request():
    time.sleep(2)
    global key_value  # 使用全局变量

    with key_lock:
        key_copy = key_value  # 复制 key_value 到线程本地变量

    print(f"Send server request with key: {key_copy}")
    
    # 构建请求数据，使用之前存储的 key 值
    data = {
        "task_id": "1111",
        "key": key_copy,
        "rank": 0,
        "members": ["member_self", "member_peer"],
        "ips": ["127.0.0.1", "127.0.0.1"],
    }

    # 将数据转换为 JSON 格式
    data_json = json.dumps(data)

    # 发送 POST 请求
    url = "http://192.168.50.162:12641/v1/service/pir/server"
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, data=data_json, headers=headers)

    # 检查响应
    if response.status_code == 200:
        print("发送 server 请求成功")
    else:
        print(f"发送 server 请求失败，状态码: {response.status_code}")
        print(response.text)  

def send_client_request():
    global key_value  
    time.sleep(2)

    with key_lock:
        key_copy = key_value
    data = {
        "task_id": "1111",
        "key": key_copy,
        "rank": 1,
        "fields": ["id"],
        "labels": ["label1", "label2"],
        "members": ["member_self", "member_peer"],
        "ips": ["127.0.0.1", "127.0.0.1"],
        "data_file": "pir_client_data_100000_0.000100_0.csv",
        "query_ids": "00003594900003594,00015099900015099",
        "callback_url": "http://192.168.50.162:5000/client_callback"
    }

    data_json = json.dumps(data)

    url = "http://192.168.50.162:12641/v1/service/pir/client"
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, data=data_json, headers=headers)

    if response.status_code == 200:
        print("发送 client 请求成功")
    else:
        print(f"发送 client 请求失败，状态码: {response.status_code}")
        print(response.text)


def run_request():
    send_setup()
    send_server_request()
    send_client_request()

def execute_requests():
    # 创建十个线程来执行 run_request 函数
    with concurrent.futures.ThreadPoolExecutor(max_workers=100) as executor:
        # 提交十个任务给线程池
        futures = [executor.submit(run_request) for _ in range(100)]
        # 等待所有任务完成
        concurrent.futures.wait(futures)

if __name__ == '__main__':
    # send_setup_thread = threading.Thread(target=run_request)
    # send_setup_thread.start()  

    # app_thread = threading.Thread(target=app.run, kwargs={"host": '0.0.0.0', "port": 5000})
    # app_thread.start()  

    # send_setup_thread.join()  
    # app_thread.join()  

    app_thread = threading.Thread(target=app.run, kwargs={"host": '0.0.0.0', "port": 5000})
    app_thread.start()  

    execute_requests()

    app_thread.join()