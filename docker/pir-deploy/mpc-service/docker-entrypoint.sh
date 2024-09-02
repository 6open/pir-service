#!/bin/bash
echo "dummy-password" | gocryptfs /data/pir_encrypted_data /data/pir_meta
exec "$@"

# 运行其他服务
nginx &            # 后台运行nginx
python3 /app/s3-proxy.py &  # 后台运行s3-proxy
/app/http_server           # 前台运行http_server

# 保持容器运行
wait