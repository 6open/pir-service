#!/bin/bash

server_addr="http://192.168.50.162:12651"
client_addr="http://192.168.50.192:12651"
callback_addr="http://192.168.50.162:5000"
key="12258161523621378815"

curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "callback_url": "'${callback_addr}'/setup_callback",
    "data_file": "psi_1.csv",
    "fields": ["id"],
    "labels": ["name"]
}' ${server_addr}/v1/service/pir/setup


curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "key": "'${key}'",
    "rank": 0,
    "members": ["server_ip", "client_ip"],
    "ips": ["192.168.50.162", "192.168.50.192"]
}' ${server_addr}/v1/service/pir/server

# "labels": ["mean perimeter","mean area","mean texture","mean radius"],
curl -X POST -H "Content-Type: application/json" -d '{
    "task_id": "1111",
    "key": "'${key}'",
    "rank": 1,
    "fields": ["id"],
    "labels": ["name"],
    "members": ["server_ip", "client_ip"], 
    "ips": ["192.168.50.162", "192.168.50.192"],
    "query_ids": "178390983532242238,864362730908965398",
    "callback_url": "'${callback_addr}'/client_callback"
}' ${client_addr}/v1/service/pir/client
