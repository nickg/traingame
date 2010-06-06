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

#include "TrackCommon.hpp"
#include "ILogger.hpp"
#include "Maths.hpp"
#include "IMesh.hpp"
#include "Matrix.hpp"

#include <cmath>
#include <map>

namespace {
   const float RAIL_WIDTH = 0.05f;
   const float GAUGE = 0.5f;

   const int SLEEPERS_PER_UNIT = 4;

   const float SLEEPER_LENGTH = 0.8f;

   const Colour METAL = makeColour(0.5f, 0.5f, 0.5f);
}

IMeshBufferPtr SleeperHelper::sleeperBuf;

IMeshBufferPtr SleeperHelper::generateSleeperMeshBuffer()
{
   IMeshBufferPtr buf = makeMeshBuffer();

   const Colour brown = makeColour(0.5f, 0.3f, 0.0f);

   const float sleeperWidth = 0.1f;
   const float sleeperDepth = 0.05f;
   const float sleeperOff = sleeperWidth / 2.0f;

   const float r = SLEEPER_LENGTH / 2.0f;

   // Top
   buf->addQuad(makeVector(-sleeperOff, sleeperDepth, -r),
      makeVector(-sleeperOff, sleeperDepth, r),
      makeVector(sleeperOff, sleeperDepth, r),
      makeVector(sleeperOff, sleeperDepth, -r),
      brown);

   // Side 1
   buf->addQuad(makeVector(sleeperOff, sleeperDepth, -r),
      makeVector(sleeperOff, 0.0f, -r),
      makeVector(-sleeperOff, 0.0f, -r),
      makeVector(-sleeperOff, sleeperDepth, -r),
      brown);
      
   // Side 2
   buf->addQuad(makeVector(-sleeperOff, sleeperDepth, r),
      makeVector(-sleeperOff, 0.0f, r),
      makeVector(sleeperOff, 0.0f, r),
      makeVector(sleeperOff, sleeperDepth, r),
      brown);

   // Front
   buf->addQuad(makeVector(sleeperOff, 0.0f, r),
      makeVector(sleeperOff, 0.0f, -r),
      makeVector(sleeperOff, sleeperDepth, -r),
      makeVector(sleeperOff, sleeperDepth, r),
      brown);
      
   // Back
   buf->addQuad(makeVector(-sleeperOff, sleeperDepth, r),
      makeVector(-sleeperOff, sleeperDepth, -r),
      makeVector(-sleeperOff, 0.0f, -r),
      makeVector(-sleeperOff, 0.0f, r),
      brown);

   return buf;
}

void SleeperHelper::mergeSleeper(IMeshBufferPtr buf,
   Vector<float> off, float yAngle) const
{
   if (!sleeperBuf)
      sleeperBuf = generateSleeperMeshBuffer();

   buf->merge(sleeperBuf, off, yAngle);
}

void BezierHelper::buildOneBezierRail(const BezierCurve<float>& func,
   IMeshBufferPtr buf, float p)
{
   const float step = 0.1f;

   for (float t = 0.0f; t < 1.0f; t += step) {

      Vector<float> v1 = func.offset(t, p);
      Vector<float> v2 = func.offset(t + step, p);
              
      v1.z -= RAIL_WIDTH / 2.0f;
      v2.z -= RAIL_WIDTH / 2.0f;

      // Top of rail
      buf->addQuad(makeVector(v1.x, v1.y + track::RAIL_HEIGHT, v1.z),
         makeVector(v1.x, v1.y + track::RAIL_HEIGHT, v1.z + RAIL_WIDTH),
         makeVector(v2.x, v2.y + track::RAIL_HEIGHT, v2.z + RAIL_WIDTH),
         makeVector(v2.x, v2.y + track::RAIL_HEIGHT, v2.z),
         METAL);

      // Outer edge
      buf->addQuad(makeVector(v2.x, v2.y + track::RAIL_HEIGHT, v2.z),
         makeVector(v2.x , v2.y, v2.z),
         makeVector(v1.x, v1.y, v1.z),
         makeVector(v1.x, v1.y + track::RAIL_HEIGHT, v1.z),
         METAL);

      // Inner edge
      buf->addQuad(makeVector(v1.x, v1.y + track::RAIL_HEIGHT, v1.z + RAIL_WIDTH),
         makeVector(v1.x, v1.y, v1.z + RAIL_WIDTH),
         makeVector(v2.x , v2.y, v2.z + RAIL_WIDTH),
         makeVector(v2.x, v2.y + track::RAIL_HEIGHT, v2.z + RAIL_WIDTH),
         METAL);
   }
}

IMeshBufferPtr BezierHelper::makeBezierRailMesh(
   const BezierCurve<float>& func) const
{
   IMeshBufferPtr buf = makeMeshBuffer();
   
   buildOneBezierRail(func, buf, GAUGE/2.0f);
   buildOneBezierRail(func, buf, -GAUGE/2.0f);

   return buf;
}

IMeshBufferPtr StraightTrackHelper::railBuf;

IMeshBufferPtr StraightTrackHelper::generateRailMeshBuffer()
{
   IMeshBufferPtr buf = makeMeshBuffer();

   // Top side
   buf->addQuad(makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      METAL);
      
   // Outer side
   buf->addQuad(makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      makeVector(-RAIL_WIDTH/2.0f, 0.0f, 0.0f),
      makeVector(-RAIL_WIDTH/2.0f, 0.0f, 1.0f),
      makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      METAL);
   
   // Inner side
   buf->addQuad(makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      makeVector(RAIL_WIDTH/2.0f, 0.0f, 1.0f),
      makeVector(RAIL_WIDTH/2.0f, 0.0f, 0.0f),
      makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      METAL);

   return buf;
}         

void StraightTrackHelper::mergeOneRail(IMeshBufferPtr buf,
   Vector<float> off, float yAngle) const
{
   if (!railBuf)
      railBuf = generateRailMeshBuffer();
   
   buf->merge(railBuf, off, yAngle);
}
   
void StraightTrackHelper::mergeStraightRail(IMeshBufferPtr buf,
   Vector<float> off, float yAngle) const
{
   Matrix<float, 4> r = Matrix<float, 4>::rotation(yAngle, 0.0f, 1.0f, 0.0f);
   
   off += r.transform(makeVector(-GAUGE/2.0f, 0.0f, -0.5f));
   mergeOneRail(buf, off, yAngle);

   off += r.transform(makeVector(GAUGE, 0.0f, 0.0f));
   mergeOneRail(buf, off, yAngle);
}

CurvedTrackHelper::CurvedRailMeshMap CurvedTrackHelper::curvedRailMeshes;

// Move to the origin of a curved section of track
void CurvedTrackHelper::transformToOrigin(Vector<float>& off,
   int baseRadius, track::Angle startAngle) const
{
   off += makeVector(
      (baseRadius-1)*-sin(degToRad(startAngle)) - 0.5f,
      0.0f,
      (baseRadius-1)*-cos(degToRad(startAngle)) - 0.5f);

   // There *must* be a way to incorporate this in the above translation
   // as a neat formula, but I really can't think of it
   // This is a complete a hack, but whatever...
   if (startAngle >= 90 && startAngle <= 180)
      off += makeVector(0.0f, 0.0f, 1.0f);
   
   if (startAngle >= 180 && startAngle <= 270)
      off += makeVector(1.0f, 0.0f, 0.0f);
}

void CurvedTrackHelper::generateCurvedRailMesh(IMeshBufferPtr buf,
   int baseRadius, RailType type)
{
   const float edgeWidth = (1 - GAUGE - RAIL_WIDTH)/2.0f;
   const float R = static_cast<float>(baseRadius) - edgeWidth
      - (type == OUTER_RAIL ? 0 : GAUGE);
   const float r = R - RAIL_WIDTH;
      
   const float step = M_PI / 2.0f / 10.0f;
      
   // Top of rail
   for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
      buf->addQuad(makeVector(r * cos(theta), 0.1f, r * sin(theta)), 
         makeVector(r * cos(theta + step), 0.1f, r * sin(theta + step)),
         makeVector(R * cos(theta + step), 0.1f, R * sin(theta + step)),
         makeVector(R * cos(theta), 0.1f, R * sin(theta)),
         METAL);
   }
      
   // Outer edge
   for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
      const float sinT = sin(theta);
      const float cosT = cos(theta);
      const float sinT1 = sin(theta + step);
      const float cosT1 = cos(theta + step);

      buf->addQuad(// Vertices
         makeVector(R * cosT1, 0.1f, R * sinT1),
         makeVector(R * cosT1, 0.0f, R * sinT1),
         makeVector(R * cosT, 0.0f, R * sinT),
         makeVector(R * cosT, 0.1f, R * sinT),

         // Normals
         makeVector(cosT1, 0.0f, sinT1),
         makeVector(cosT1, 0.0f, sinT1),
         makeVector(cosT, 0.0f, sinT),
         makeVector(cosT, 0.0f, sinT),

         METAL);
   }
      
   // Inner edge
   for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
      const float sinT = sin(theta);
      const float cosT = cos(theta);
      const float sinT1 = sin(theta + step);
      const float cosT1 = cos(theta + step);

      buf->addQuad(// Vertices
         makeVector(r * cosT, 0.1f, r * sinT),
         makeVector(r * cosT, 0.0f, r * sinT),
         makeVector(r * cosT1, 0.0f, r * sinT1),
         makeVector(r * cosT1, 0.1f, r * sinT1),

         // Normals
         makeVector(-cosT, 0.0f, -sinT),
         makeVector(-cosT, 0.0f, -sinT),
         makeVector(-cosT1, 0.0f, -sinT1),
         makeVector(-cosT1, 0.0f, -sinT1),

         METAL);
   }
}
   
void CurvedTrackHelper::mergeCurvedRail(IMeshBufferPtr buf, int baseRadius,
   Vector<float> off, float yAngle)
{
   IMeshBufferPtr railBuf;
      
   CurvedRailMeshMap::iterator it = curvedRailMeshes.find(baseRadius);
   if (it != curvedRailMeshes.end())
      railBuf = (*it).second;
   else {
      railBuf = makeMeshBuffer();
         
      generateCurvedRailMesh(railBuf, baseRadius, INNER_RAIL);
      generateCurvedRailMesh(railBuf, baseRadius, OUTER_RAIL);
            
      curvedRailMeshes[baseRadius] = railBuf;
   }

   buf->merge(railBuf, off, yAngle);
}

void CurvedTrackHelper::mergeCurvedTrack(IMeshBufferPtr buf, Vector<float> off,
   int baseRadius, track::Angle startAngle, track::Angle endAngle) const
{
   transformToOrigin(off, baseRadius, startAngle);

   mergeCurvedRail(buf, baseRadius, off, static_cast<float>(startAngle));

   const float length =
      degToRad(static_cast<float>(endAngle - startAngle)) * baseRadius;
   const int numSleepers = static_cast<int>(length * SLEEPERS_PER_UNIT);
   const float sleeperAngle =
      static_cast<float>(endAngle - startAngle) / numSleepers;
   
   for (int i = 0; i < numSleepers; i++) {
      
      float yAngle = static_cast<float>(startAngle) + (i + 0.5f)*sleeperAngle;
      Vector<float> t =
         makeVector(0.0f, 0.0f, static_cast<float>(baseRadius) - 0.5f);

      mergeSleeper(buf, off + rotateY(t, yAngle), yAngle);
   }
}
