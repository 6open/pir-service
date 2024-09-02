#docker run  -v bin:/home/admin/bin -it mpc:1.0 bash
# MPC_DIR=/data2/lin USER=lk ./run_image_dev.sh
USER=lk-dev-1k
export MPC_DEV_DIR="/data/lk/mpc_dev"
docker-compose -f docker-compose.yml up -d $USER