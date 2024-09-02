import boto3
import polars as pl
from io import BytesIO
import logging
from flask import Flask, jsonify, request
import json
import requests

app = Flask(__name__)

G_DIRECTORY="/data/storage/dataset/pre_deal_result/"

def list_bucket_name(s3):
# 列出存储桶
    response = s3.list_buckets()
    print("存储桶列表:")
    for bucket in response['Buckets']:
        print(f'  {bucket["Name"]}')

def list_file_name(s3, bucket_name):
    # 列出存储桶中的所有文件
    response = s3.list_objects_v2(Bucket=bucket_name)
    print("文件列表：")
    for obj in response.get('Contents', []):
        print(obj['Key'])
    
# 上传文件
# s3.upload_file(s3_info["localFile"], s3_info["bucketName"], s3_info["remoteFile"])


@app.route('/healthz')
def health_api():
    return 'OK'

# 读取文件
@app.route('/v1/service/pir/setup', methods=['POST'])
def load_s3_data():
    requset_data = request.json
    task_id = request.json.get('task_id')
    data_info = request.json.get('data_file')
    
    if 's3Info' in data_info:
        s3_info = json.loads(data_info)
        s3_info = s3_info['s3Info']
        print(s3_info)
        s3 = boto3.client(
            's3',
            endpoint_url=s3_info["endpoint"],
            aws_access_key_id=s3_info["accessKey"],
            aws_secret_access_key=s3_info["secretKey"]
        )
        # list_bucket_name(s3)
        # list_file_name(s3, s3_info['bucketName'])
        # s3.upload_file("/data/storage/dataset/pre_deal_result/pir_server_data_100.csv", s3_info["bucketName"], "pir_server_data_100.csv")
        # list_file_name(s3, s3_info['bucketName'])
        
        obj = s3.get_object(Bucket=s3_info["bucketName"], Key=s3_info["fileName"])
        df = pl.read_csv(BytesIO(obj['Body'].read()))
        out_file = G_DIRECTORY + s3_info["fileName"]
        df.write_csv(out_file)
        print(df)
        requset_data['data_file'] = s3_info["fileName"]
    
    headers = {'Content-Type': 'application/json'}
    
    pir_server = "http://0.0.0.0:12601/v1/service/pir/setup"
    print(requset_data)
    callback_result = requests.post(pir_server, headers=headers, json=requset_data)
    
    if callback_result.status_code == 200:
        print("请求成功")
        response = {
            "status" : callback_result.status_code,
            "task_id" : task_id
        }
    else:
        print(f"请求失败，状态码：{callback_result.status_code}")
        response = {
            "status_code" : callback_result.status_code,
            "task_id" : task_id
        }
    print("callback_result ", callback_result)

    return jsonify(response)


if __name__ == '__main__':
    print(".....load-s3data.....")
    logging.basicConfig(filename='data/s3-proxy.log', level=logging.INFO, format='%(asctime)s %(levelname)s %(message)s')
    app.run(debug=False, host='0.0.0.0', port=12602)


