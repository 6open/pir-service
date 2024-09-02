#!/bin/bash

http_res=$(curl -s http://127.0.0.1:12601/healthz | jq -r '.message' 2> /dev/null)

http_status=0
echo ${http_res}
if [[ "$http_res" == "OK" ]]; then
    echo "HTTP is OK"
else
    echo "HTTP is failed"
    http_status=1
fi

if [[ $http_status -eq 0 ]]; then
    exit 0
else
    exit 1
fi
