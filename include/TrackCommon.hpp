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
   void mergeStraightRail(IMeshBufferPtr buf,
      Vector<float> off, float yAngle) const;
   
private:
   void mergeOneRail(IMeshBufferPtr buf,
      Vector<float> off, float yAngle) const;
   
   static IMeshBufferPtr generateRailMeshBuffer();
      
   static IMeshBufferPtr railBuf;
};

class SleeperHelper {
public:
   void mergeSleeper(IMeshBufferPtr buf,
      Vector<float> off, float yAngle) const;
   
private:
   static IMeshBufferPtr generateSleeperMeshBuffer();
   
   static IMeshBufferPtr sleeperBuf;
};

class BezierHelper {
public:
   IMeshBufferPtr makeBezierRailMesh(const BezierCurve<float>& func) const;

private:
   static void buildOneBezierRail(const BezierCurve<float>& func,
      IMeshBufferPtr buf, float p);
};

class CurvedTrackHelper : private SleeperHelper {
public:
   void mergeCurvedTrack(IMeshBufferPtr buf, Vector<float> off,
      int baseRadius, track::Angle startAngle, track::Angle endAngle) const;
   
private:
   void transformToOrigin(Vector<float>& off,
      int baseRadius, track::Angle startAngle) const;
   
   enum RailType {
      INNER_RAIL, OUTER_RAIL
   };
   
   static IMeshPtr generateCurvedRailMesh(IMeshBufferPtr buf,
      int baseRadius, RailType type);
   static void mergeCurvedRail(IMeshBufferPtr buf, int baseRadius,
      Vector<float> off, float yAngle);

   typedef map<int, IMeshBufferPtr> CurvedRailMeshMap;
   static CurvedRailMeshMap curvedRailMeshes;
};

// Track constants
namespace track {
   const float RAIL_HEIGHT = 0.1f;
}

#endif
