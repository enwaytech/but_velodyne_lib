#! /usr/bin/env python

# ~/workspace/but_velodyne_lib/scripts/poses_to_graph.py < 03-poses-cls-m10.txt > 04-cls-subseq.unclosed.graph; ~/workspace/but_velodyne_lib/scripts/pose_to_edge.py --src_index_from 253 --src_index_to 495 --trg_index_from 1849 --trg_index_to 2120 -p 03-poses-cls-m10.txt -r forward.1-to-backward.9.poses -g 04-cls-subseq.unclosed.graph >>04-cls-subseq.unclosed.graph; cat loop.txt >>04-cls-subseq.unclosed.graph; ~/apps/SLAM_plus_plus_v2.10/bin/slam_plus_plus -i 04-cls-subseq.unclosed.graph --pose-only --no-detailed-timing; ~/workspace/but_velodyne_lib/bin/slampp-solution-to-poses < solution.txt > 04-cls-subseq.closed.txt; ~/workspace/but_velodyne_lib/bin/build-3d-model -p 04-cls-subseq.closed.txt $(ls fixed-by-03-poses-cls-m10/*.pcd | sort) -o 04-cls-subseq.closed.pcd; pcl_viewer 04-cls-subseq.closed.pcd.rgb.pcd 

import sys
import argparse

from odometry_cnn_data import Odometry, Edge3D, load_kitti_poses, get_delta_odometry

def printEdge(poses, reg_pose, src_index, trg_index):
    src_pose = poses[src_index]
    trg_pose = poses[trg_index]
    t = src_pose.inv() * (reg_pose * trg_pose)
    print Edge3D(src_index, trg_index, t.dof)

def printEdges(poses, reg_pose, src_index_from, src_index_to, trg_index_from, trg_index_to):
    if src_index_from <= src_index_to and trg_index_from <= trg_index_to:
        src_middle_index = (src_index_from + src_index_to) / 2
        trg_middle_index = (trg_index_from + trg_index_to) / 2

        printEdges(poses, reg_pose, src_index_from, src_middle_index-1, trg_index_from, trg_middle_index-1)
        printEdge(poses, reg_pose, src_middle_index, trg_middle_index)
        printEdges(poses, reg_pose, src_middle_index+1, src_index_to, trg_middle_index+1, trg_index_to)

def printClosestEdge(poses, reg_pose, src_index_from, src_index_to, trg_index_from, trg_index_to):
    min_dist = 1000*1000
    for src_i in range(src_index_from, src_index_to+1):
        for trg_i in range(trg_index_from, trg_index_to):
            dist = poses[src_i].distanceTo(poses[trg_i])
            if dist < min_dist:
                min_dist = dist
                min_src_i = src_i
                min_trg_i = trg_i
    printEdge(poses, reg_pose, min_src_i, min_trg_i)

def get_max_vertex(graph_file):
    max_vertex = -1
    for line in open(graph_file).readlines():
        tokens = line.split()
        src_vertex = int(tokens[1])
        trg_vertex = int(tokens[2])
        max_vertex = max(max_vertex, max(src_vertex, trg_vertex))
    return max_vertex

def printEdgesToNewVertex(poses, index_from, index_to, new_vertex):
    start_pose = poses[index_from]
    for i in range(index_from, index_to+1, 50):
        t = start_pose.inv() * poses[i]
        print Edge3D(new_vertex, i, t.dof)

parser = argparse.ArgumentParser(description="Pose from subsequence registration to EDGE3D")
parser.add_argument("--src_index_from", dest="src_index_from", type=int, required=True)
parser.add_argument("--src_index_to", dest="src_index_to", type=int, required=True)
parser.add_argument("--trg_index_from", dest="trg_index_from", type=int, required=True)
parser.add_argument("--trg_index_to", dest="trg_index_to", type=int, required=True)
parser.add_argument("-p", "--poses", dest="poses", type=str, required=True)
parser.add_argument("-r", "--registration_pose", dest="registration_pose", type=str, required=True)
parser.add_argument("-g", "--graph_file", dest="graph_file", type=str, required=True)
args = parser.parse_args()

reg_poses = load_kitti_poses(args.registration_pose)
assert len(reg_poses) == 1
reg_pose = reg_poses[0]
poses = load_kitti_poses(args.poses)

printEdges(poses, reg_pose, args.src_index_from, args.src_index_to, args.trg_index_from, args.trg_index_to)
sys.exit(0)

#printEdge(poses, reg_pose, args.src_index_from, args.trg_index_from)
#printEdge(poses, reg_pose, args.src_index_to, args.trg_index_to)

#printClosestEdge(poses, reg_pose, args.src_index_from, args.src_index_to, args.trg_index_from, args.trg_index_to)

new_src_vertex = get_max_vertex(args.graph_file)+1
new_trg_vertex = new_src_vertex+1
printEdgesToNewVertex(poses, args.src_index_from, args.src_index_to, args.src_index_from)
printEdgesToNewVertex(poses, args.trg_index_from, args.trg_index_to, args.trg_index_from)
print Edge3D(args.src_index_from, args.trg_index_from, reg_pose.dof)