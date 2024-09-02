
from flask import Flask, request, jsonify

app = Flask(__name__)

@app.route('/setup_callback', methods=['POST'])
@app.route('/client_callback', methods=['POST'])
def setup_callback():
    data = request.get_json()  # 获取通过回调URL发送的数据
    # # 获取 "key" 字段的值
    # key_value = data.get('data_result', {}).get('key')
    # if key_value:
    #     print(f"Received callback data with key: {key_value}")

    # # 在这里处理回调数据
    # print("Received callback data:")
    print(data)
    
    # 执行处理操作，生成响应数据
    response_data = {
        "code": 0,
        "spend": 100
    }
    return jsonify(response_data)  # 返回 JSON 响应数据

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000)
