#!/bin/bash
# docker build -f workDockerfile --build-arg TOOL_PATH="docker" --tag 192.168.50.32/mpc-dev/mpc-pir:1.0 .
# docker push 192.168.50.32/mpc-dev/mpc-pir:1.0

# s3版本
docker build -f workDockerfile --build-arg TOOL_PATH="docker" --tag 192.168.50.32/mpc-dev/mpc-pir:1.0.2 .
docker push 192.168.50.32/mpc-dev/mpc-pir:1.0.2

# docker build -f Dockerfile --build-arg TOOL_PATH="docker" --tag 192.168.50.32/mpc-dev/mpc-pir:0.1 .
# docker push 192.168.50.32/mpc-dev/mpc-pir:0.1
