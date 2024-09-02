#!/bin/bash
rm -f mpc_v1.0.tar.gz
docker save  mpc:1.0 |gzip > mpc_v1.0.tar.gz
tar -czvf mpc_depoly_v1.0.tar.gz  run_image_ops.sh docker/ops/docker-compose.yml bin
tar -czvf mpc-service.tar.gz mpc-service

#scp mpc_depoly_v1.0.tar.gz mpc_v1.0.tar.gz hz_dev@192.168.10.16:/home/ftp/nvxclouds/apollo_v2.3.0/
#unpacked
#mkdir mpc_iamge
#tar -xzvf mpc.tar.gz -C mpc_iamge
#rm -f mpc.tar.gz
#cd mpc_iamge
#docker load < mpc.tar
#rm -f mpc.tar
#./run_image_ops.sh
# 

Nvx@123$
#
#copy to ftp
scp mpc-service.tar.gz devops@192.168.10.16:/home/ftp/nvxclouds/apollo_v2.3.4/apollo/apollo-app 
#copy to server
scp mpc-service.tar.gz app@192.168.10.98:/home/app/apollo_v2.3.4/apollo/apollo-app/