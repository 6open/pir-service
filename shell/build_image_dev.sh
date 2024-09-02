#!/bin/bash

docker build -f ./docker/dev/Dockerfile --build-arg TOOL_PATH="docker" --tag mpc:lin_dev .