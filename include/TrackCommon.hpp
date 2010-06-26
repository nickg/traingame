//
//  Copyright (C) 2009-2010  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INC_TRACK_COMMON_HPP
#define INC_TRACK_COMMON_HPP

#include "ITrackSegment.hpp"
#include "IMesh.hpp"
#include "BezierCurve.hpp"

#include <map>

class StraightTrackHelper {
public:
   void merge_straight_rail(IMeshBufferPtr buf,
      Vector<float> off, float y_angle) const;
   
private:
   void merge_one_rail(IMeshBufferPtr buf,
      Vector<float> off, float y_angle) const;
   
   static IMeshBufferPtr generate_rail_meshBuffer();
      
   static IMeshBufferPtr rail_buf;
};

class SleeperHelper {
public:
   void merge_sleeper(IMeshBufferPtr buf,
      Vector<float> off, float y_angle) const;
   
private:
   static IMeshBufferPtr generate_sleeper_meshBuffer();
   
   static IMeshBufferPtr sleeper_buf;
};

class BezierHelper {
public:
   IMeshBufferPtr make_bezier_railMesh(const BezierCurve<float>& func) const;

private:
   static void build_one_bezierRail(const BezierCurve<float>& func,
      IMeshBufferPtr buf, float p);
};

class CurvedTrackHelper : private SleeperHelper {
public:
   void merge_curved_track(IMeshBufferPtr buf, Vector<float> off,
      int base_radius, track::Angle start_angle, track::Angle end_angle) const;
   
private:
   void transform_to_origin(Vector<float>& off,
      int base_radius, track::Angle start_angle) const;
   
   enum RailType {
      INNER_RAIL, OUTER_RAIL
   };
   
   static void generate_curved_railMesh(IMeshBufferPtr buf,
      int base_radius, RailType type);
   static void merge_curved_rail(IMeshBufferPtr buf, int base_radius,
      Vector<float> off, float y_angle);

   typedef map<int, IMeshBufferPtr> CurvedRailMeshMap;
   static CurvedRailMeshMap curved_rail_meshes;
};

// Track constants
namespace track {
   const float RAIL_HEIGHT = 0.1f;
}

#endif
