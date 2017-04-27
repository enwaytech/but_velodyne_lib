#! /bin/bash

POSES_DIR=/media/files/cnn_velodyne_data/poses/
EMPTY_EVAL=/media/files/cnn_velodyne_data/results/empty-evaluation

TRANSL_DATA=/media/files/cnn_velodyne_data/results/17-joined-Rt/26-odometry_l335_transl
ROT_DATA=/media/files/cnn_velodyne_data/results/17-joined-Rt/46-odometry_l335_h6_b16_rot_shuffled_it500k
ROT_X_DATA=/media/files/cnn_velodyne_data/results/27-odometry_l335_rx_comp_hdfin-it200000-window
ROT_Y_DATA=/media/files/cnn_velodyne_data/results/13-odometry_l335_b1_shifts56_rcls_comp_fixshift-it200k-window
#ROT_Z_DATA=/media/files/cnn_velodyne_data/results/26-odometry_l335_rz_comp_hdfin_lr5e-4-it200000-window

for res_folder in $@
do
	cp $0 $res_folder
	pushd $res_folder

	rm -rf evaluation
	cp -r $EMPTY_EVAL evaluation
	for i in 00.txt  01.txt  02.txt  03.txt  04.txt  05.txt  06.txt  07.txt  08.txt  09.txt  10.txt
	do 
		echo "Merging $i"
#		~/workspace/but_velodyne_lib/scripts/merge_pose_files.py -t $TRANSL_DATA/$i -r $ROT_DATA/$i -rx $ROT_X_DATA/$i -ry $ROT_Y_DATA/$i -rz $ROT_Z_DATA/$i > evaluation/results/ivelas/data/$i
		~/workspace/but_velodyne_lib/scripts/merge_pose_files.py -t $TRANSL_DATA/$i -r $ROT_DATA/$i -rx $ROT_X_DATA/$i -ry $ROT_Y_DATA/$i > evaluation/results/ivelas/data/$i
	done
	pushd evaluation
		~/workspace/tmp-ivelas/sw/kitti-odometry-devkit/evaluate_odometry ivelas
	popd

	popd
done
