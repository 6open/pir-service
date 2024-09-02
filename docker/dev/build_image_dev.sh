#!/bin/bash

# docker build -f Dockerfile --build-arg TOOL_PATH="docker" --tag mpc:mscp-dev .

docker build -f Dockerfile --build-arg TOOL_PATH="docker" --tag mpc:mscp-base .