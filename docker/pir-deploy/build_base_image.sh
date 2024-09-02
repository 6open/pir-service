#!/bin/bash
docker build -f Dockerfile  --network=host --tag mpc-pir-base:1.0.0 .
