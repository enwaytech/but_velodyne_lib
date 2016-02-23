/*
 * Copyright (C) Brno University of Technology (BUT)
 *
 * This file is part of software developed by Robo@FIT group.
 *
 * Author: Martin Velas (ivelas@fit.vutbr.cz)
 * Supervised by: Michal Spanel & Adam Herout ({spanel|herout}@fit.vutbr.cz)
 * Date: 27/03/2015
 *
 * This file is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this file.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/program_options/errors.hpp>

#include <but_velodyne/Visualizer3D.h>
#include <but_velodyne/LineCloud.h>

using namespace std;
using namespace pcl;

namespace but_velodyne
{

LineCloud::LineCloud(const PolarGridOfClouds &polar_grid,
                     const int lines_per_cell_pair_generated,
                     CollarLinesFilter &filter_) :
    rng(cv::theRNG()),
    filter(filter_)
{
  for(int polar = 0; polar < PolarGridOfClouds::getPolarBins(); polar++) {
    for(int ring = 0; ring < VelodynePointCloud::VELODYNE_RINGS_COUNT-1; ring++) {
      //cerr << "Ring: " << ring << ", expected_range: " << VelodyneSpecification::getExpectedRange(ring, VelodyneSpecification::KITTI_HEIGHT) << endl;
      vector<PointCloudLine> lines_among_cells;
      generateLineCloudFromCell(polar_grid,
                                CellId(polar, ring),
                                lines_per_cell_pair_generated,
                                lines_among_cells);
      line_cloud.insert(line_cloud.end(),
                        lines_among_cells.begin(), lines_among_cells.end());
      for(vector<PointCloudLine>::iterator line = lines_among_cells.begin();
          line < lines_among_cells.end();
          line++) {
        Eigen::Vector3f middle = line->middle();
        line_middles.push_back(PointXYZ(middle.x(), middle.y(), middle.z()));
      }
    }
  }
}

pcl::PointCloud<pcl::PointXYZ>::Ptr LineCloud::generateDenseCloud(const int points_per_cell) const {

  pcl::PointCloud<pcl::PointXYZ>::Ptr generated_cloud(new pcl::PointCloud<pcl::PointXYZ>);
  int points_generated = PolarGridOfClouds::getPolarBins()*(VelodyneSpecification::RINGS-1)*points_per_cell;
  int points_per_line = points_generated / line_cloud.size();
  points_generated = points_per_line * line_cloud.size();
  generated_cloud->resize(points_generated);
  int cloud_index = 0;

   for(std::vector<PointCloudLine>::const_iterator line = line_cloud.begin(); line < line_cloud.end(); line++) {
     float max_factor = line->orientation.norm();
     for(int i = 0; i < points_per_line; i++) {
       float factor = rng.uniform(0.0f, max_factor);
       Eigen::Vector3f new_point = line->point + line->getOrientationOfSize(factor);
       generated_cloud->points[cloud_index++].getVector3fMap() = new_point;
     }
   }
  return generated_cloud;
}

void LineCloud::generateLineCloudAmongCells(const PolarGridOfClouds &polar_grid,
                                            CellId cell1_id, CellId cell2_id,
                                            int lines_per_cell_pair_generated,
                                            vector<PointCloudLine> &output_lines) const {
  const VelodynePointCloud *cell1 = &polar_grid[cell1_id];
  const VelodynePointCloud *cell2;
  while(true) {
    cell2 = &polar_grid[cell2_id];
    if(cell2->empty() && (cell2_id.ring < VelodyneSpecification::RINGS-1)) {
      cell2_id.ring++;
      //cerr << cell1_id << " -> " << cell2_id << endl;
    } else {
      break;
    }
  }
  int lines_to_generate = MIN(lines_per_cell_pair_generated, cell1->size()*cell2->size());

  vector<PointCloudLine> generated_lines;
  for(int i = 0; i < lines_to_generate; i++) {
    int cell1_index = rng(cell1->size());
    int cell2_index = rng(cell2->size());
    PointCloudLine generated_line(cell1->at(cell1_index),
                                  cell2->at(cell2_index));
    generated_lines.push_back(generated_line);
  }
  filter.filterLines(generated_lines, output_lines, cell1_id, cell2_id);
}

void LineCloud::generateLineCloudFromCell(const PolarGridOfClouds &polar_grid,
                               const CellId &source_cell,
                               const int lines_per_cell_pair_generated,
                               std::vector<PointCloudLine> &line_cloud) const {
  vector<CellId> target_cells = getTargetCells(source_cell);
  for(vector<CellId>::iterator target_cell = target_cells.begin(); target_cell < target_cells.end(); target_cell++) {
    generateLineCloudAmongCells(polar_grid,
                                source_cell, *target_cell,
                                lines_per_cell_pair_generated,
                                line_cloud);
  }
}

vector<CellId> LineCloud::getTargetCells(const CellId &source_cell) const {
  vector<CellId> target_cells;

  int min_polar = source_cell.polar - PolarGridOfClouds::BIN_SUBDIVISION / 2;
  int max_polar = source_cell.polar + PolarGridOfClouds::BIN_SUBDIVISION / 2;

  for(int polar = min_polar; polar <= max_polar; polar++) {
    int polar_periodic = (polar + PolarGridOfClouds::getPolarBins()) % PolarGridOfClouds::getPolarBins();
    target_cells.push_back(CellId(polar_periodic, source_cell.ring+1));
  }

  return target_cells;
}

void LineCloud::transform(const Eigen::Matrix4f &transformation, LineCloud &output) const {
  output.line_cloud.clear();
  for(std::vector<PointCloudLine>::const_iterator line = line_cloud.begin();
      line < line_cloud.end(); line++) {
    output.line_cloud.push_back(line->transform(transformation));
  }
  pcl::transformPointCloud(line_middles, output.line_middles, transformation);
}

void LineCloud::transform(const Eigen::Matrix4f &transformation) {
  for(std::vector<PointCloudLine>::iterator line = line_cloud.begin();
      line < line_cloud.end(); line++) {
    *line = line->transform(transformation);
  }
  pcl::transformPointCloud(line_middles, line_middles, transformation);
}

} /* namespace but_velodyne */
