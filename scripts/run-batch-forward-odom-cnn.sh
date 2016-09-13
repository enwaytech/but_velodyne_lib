#! /bin/bash

DEPLOY_PROTOTXT=~/workspace/ivelas-git/sw/cnn-generator/deploy-definitions/odometry_conv_only_div4_separated_deploy.prototxt
TRAINED_MODEL=~/workspace/ivelas-git/sw/cnn-generator/NETS/odometry_conv_only_div4_separated/net_snapshot_iter_500000.caffemodel
OUTPUT_DIR=/media/kitti/dataset_odometry_velodyne_odom_cnn_data/results/12-odometry_conv_only_div4_separated

DATA_DIR=/media/kitti/dataset_odometry_velodyne_ground_fake_ann/00-10/  
POSES_DIR=/media/kitti/dataset_odometry_poses/poses
SCRIPT_DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

mkdir -p $OUTPUT_DIR
for i in $(ls $DATA_DIR)
do 
	echo $i; 
	$SCRIPT_DIR/odometry_forward.py -p $DEPLOY_PROTOTXT -m $TRAINED_MODEL $(ls $DATA_DIR/$i/velodyne/*.yaml.gz | sort) -i $POSES_DIR/$i.txt -g $OUTPUT_DIR/$i.graph | tee $OUTPUT_DIR/$i.txt
done

cp $DEPLOY_PROTOTXT $TRAINED_MODEL $OUTPUT_DIR
cp $SCRIPT_DIR/odometry_forward.py $OUTPUT_DIR/schema.txt