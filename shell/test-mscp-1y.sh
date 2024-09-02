#!/bin/bash

server_addr="http://192.168.50.162:12641"
callback_addr="http://192.168.50.162:5000"
key="00003594900003594"

curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "callback_url": "'${callback_addr}'/setup_callback",
    "data_file": "pir_server_data_100000000.csv",
    "fields": ["id"],
    "labels": ["label1", "label2"]
}' ${server_addr}/v1/service/pir/setup


curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "key": "'${key}'",
    "rank": 0,
    "members": ["member_self", "member_peer"],
    "ips": ["127.0.0.1", "127.0.0.1"]
}' ${server_addr}/v1/service/pir/server

# "labels": ["mean perimeter","mean area","mean texture","mean radius"],
curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "key": "${key}'",
    "rank": 1,
    "fields": ["id"],
    "labels": ["label1", "label2"],
    "members": ["member_self", "member_peer"], 
    "ips": ["127.0.0.1", "127.0.0.1"],
    "query_ids": "10001,10002",
    "callback_url": "'${callback_addr}'/client_callback"
}' ${server_addr}/v1/service/pir/client




curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "callback_url": "192.168.50.192:5000/setup_callback",
    "data_file": "pir_server_data_100000000.csv",
    "fields": ["id"],
    "labels": ["label"]
}' http://192.168.50.162:12641/v1/service/pir/setup

# 11351687322973111953

curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "key": "11351687322973111953",
    "rank": 0,
    "members": ["member_self", "member_peer"],
    "ips": ["192.168.50.162", "192.168.50.192"]
}' http://192.168.50.162:12641/v1/service/pir/server

# "labels": ["mean perimeter","mean area","mean texture","mean radius"],
curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "key": "11351687322973111953",
    "rank": 1,
    "fields": ["id"],
    "labels": ["label"],
    "members": ["member_self", "member_peer"], 
    "ips": ["192.168.50.162", "192.168.50.192"],
    "query_ids": "00000001900000001",
    "callback_url": "192.168.50.192:5000/client_callback"
}' http://192.168.50.192:12641/v1/service/pir/client