# 使用 Ubuntu 作为基础镜像
FROM ubuntu:22.04

# 更新包列表并安装 gocryptfs 和其他必要的工具
RUN apt-get update && \
    apt-get install -y gocryptfs fuse && \
    apt-get install -y python3 python3-dev python3-pip curl nginx vim&& \
    apt-get clean

# 创建加密目录和挂载点
RUN mkdir -p /data/pir_encrypted_data /data/pir_meta /check_health

# 初始化 gocryptfs
RUN echo "dummy-password" | gocryptfs -init /data/pir_encrypted_data

# RUN mkdir /check_health && cp /home/admin/bin/check_http_health.sh /check_health

COPY mpc-service /app
WORKDIR /app

RUN mv /app/check_http_health.sh /check_health && mv nginx.conf /etc/nginx  && pip3 install -r requirements.txt -i https://pypi.tuna.tsinghua.edu.cn/simple

# 设置容器入口点
# ENTRYPOINT ["sh docker-entrypoint.sh && nginx && python3 s3-proxy.py && ./http_server"]

# 默认命令
CMD ["bash"]

