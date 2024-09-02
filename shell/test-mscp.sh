#!/bin/bash

server_addr="http://192.168.50.162:12641"
callback_addr="http://192.168.50.162:5000"
key="292035638260484487"

curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "callback_url": "'${callback_addr}'/setup_callback",
    "data_file": "data_set_1720412942731096065_V1.csv",
    "fields": ["id"],
    "labels": ["mean perimeter","mean area","mean texture","mean radius"]
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
    "key": "'${key}'",
    "rank": 1,
    "fields": ["id"],
    "labels": ["mean perimeter","mean area","mean texture","mean radius"],
    "members": ["member_self", "member_peer"], 
    "ips": ["127.0.0.1", "127.0.0.1"],
    "query_ids": "10001,10002",
    "callback_url": "'${callback_addr}'/client_callback"
}' ${server_addr}/v1/service/pir/client
