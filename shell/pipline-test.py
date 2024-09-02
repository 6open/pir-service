import threading
import requests
import json
from flask import Flask, request, jsonify
import time
import csv
import json

app = Flask(__name__)

key_value = None
key_lock = threading.Lock()  # 互斥锁
server1_ip = "192.168.50.162"
server1_addr = "http://192.168.50.162:12651"
server2_ip = "192.168.50.161"
server2_addr = "http://192.168.50.161:12651"
callback_addr = "http://192.168.50.162:5000"

client_ip = "192.168.50.192"
client_addr = "http://192.168.50.192:12651"

pir_result_file = "tmp_result.csv"

def clean_string(s):
    # 清除非打印字符和空字符
    return ''.join([c for c in s if c.isprintable() and c != '\x00'])

def read_id_field_from_csv(file_path, rows=None):
    ids = []  # 用于存储读取的 id 数据
    with open(file_path, newline='') as csvfile:
        reader = csv.DictReader(csvfile)
        for index, row in enumerate(reader):
            # 读取每一行的 id 字段数据
            id_value = row.get('id')
            if id_value:  # 如果 id 字段存在，则添加到列表中
                ids.append(id_value)
            
            if rows  and index + 1 >= rows:  # 读取指定的行数后停止
                break
    
    # 将 id 数据格式化为指定形式的字符串
    formatted_ids = ','.join(ids)
    return formatted_ids

def write_to_csv_if_values(filename, data):
    with open(filename, mode='w', newline='') as csvfile:
        fieldnames = ['id', 'name']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames, escapechar='\\')

        writer.writeheader()
        for item in data:
            data_content = item.get('data_content')
            if data_content:
                for key, value in data_content.items():
                    if 'values' in value:
                        cleaned_name = clean_string(value['values'][0])
                        writer.writerow({'id': key, 'name': cleaned_name})
                    
@app.route('/setup_callback', methods=['POST'])
def setup_callback():
    global key_value
    data = request.get_json()  # 获取通过回调URL发送的数据
    
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
        write_to_csv_if_values(pir_result_file, result_value)

    # 执行处理操作，生成响应数据
    response_data = {
        "code": 0,
        "spend": 100
    }
    return jsonify(response_data)  # 返回 JSON 响应数据

def send_setup(server_addr):
    global callback_addr
    time.sleep(3)
    data = {
        "task_id": "1111",
        "callback_url": callback_addr + "/setup_callback",
        "data_file": "data.csv",
        "fields": ["id"],
        "labels": ["name"]
    }

    data_json = json.dumps(data)

    url = server_addr + "/v1/service/pir/setup"
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

def send_server_request(server_addr, server_ip):
    global client_ip
    global key_value
    time.sleep(2)
    
    with key_lock:
        key_copy = key_value  # 复制 key_value 到线程本地变量

    print(f"Send server request with key: {key_copy}")
    
    # 构建请求数据，使用之前存储的 key 值
    data = {
        "task_id": "1111",
        "key": key_copy,
        "members": ["server_ip", "client_ip"],
        "ips": [server_ip, client_ip],
    }

    # 将数据转换为 JSON 格式
    data_json = json.dumps(data)

    # 发送 POST 请求
    url = server_addr + "/v1/service/pir/server"
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, data=data_json, headers=headers)

    # 检查响应
    if response.status_code == 200:
        print("发送 server 请求成功")
    else:
        print(f"发送 server 请求失败，状态码: {response.status_code}")
        print(response.text)  

def send_client_request(query_id, server_ip):
    global key_value  
    global client_addr, client_ip
    time.sleep(2)

    with key_lock:
        key_copy = key_value
    data = {
        "task_id": "1111",
        "key": key_copy,
        "rank": 1,
        "fields": ["id"],
        "labels": ["name"],
        "members": ["server_ip", "client_ip"],
        "ips": [server_ip, client_ip],
        "query_ids": query_id,
        
        "callback_url": callback_addr + "/client_callback"
    }
    # "query_ids": "178390983532242238,864362730908965398",
    data_json = json.dumps(data)

    url = "http://192.168.50.192:12651/v1/service/pir/client"
    headers = {"Content-Type": "application/json"}
    response = requests.post(url, data=data_json, headers=headers)

    if response.status_code == 200:
        print("发送 client 请求成功")
    else:
        print(f"发送 client 请求失败，状态码: {response.status_code}")
        print(response.text)


def run_request():
    global server1_addr, server2_addr
    global server1_ip, server2_ip
    global pir_result_file
    start_time = time.time()
    send_setup(server1_addr)
    send_server_request(server1_addr, server1_ip)
    # query_id = "178390983532242238"
    query_id = read_id_field_from_csv("/data/storage/data.csv", 10)
    send_client_request(query_id, server1_ip)
    
    
    send_setup(server2_addr)
    send_server_request(server2_addr, server2_ip)
    query_id = read_id_field_from_csv(pir_result_file, 10)
    pir_result_file = "final.csv"
    send_client_request(query_id, server2_ip)
    end_time = time.time()
    execution_time = end_time - start_time
    print(execution_time)
    
if __name__ == '__main__':
    send_setup_thread = threading.Thread(target=run_request)
    send_setup_thread.start()  

    app_thread = threading.Thread(target=app.run, kwargs={"host": '0.0.0.0', "port": 5000})
    app_thread.start()  

    send_setup_thread.join()  
    app_thread.join()  
