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

#include <GL/gl.h>

namespace {
   const float RAIL_WIDTH = 0.05f;
   const float GAUGE = 0.5f;

   const int SLEEPERS_PER_UNIT = 4;

   const float SLEEPER_LENGTH = 0.8f;

   IMeshPtr theSleeperMesh, theRailMesh;

   typedef map<int, IMeshPtr> CurvedRailMeshMap;
   CurvedRailMeshMap theCurvedRailMeshes;
   
   const Colour METAL = makeColour(0.5f, 0.5f, 0.5f);
   
   IMeshBufferPtr generateSleeperMeshBuffer()
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

   void generateSleeperMesh()
   {
      theSleeperMesh = makeMesh(generateSleeperMeshBuffer());
   }
   
   IMeshBufferPtr generateRailMeshBuffer()
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

   void generateRailMesh()
   {
      theRailMesh = makeMesh(generateRailMeshBuffer());
   }

   void buildOneBezierRail(const BezierCurve<float>& func,
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

   // Generate a rail mesh from a Bezier curve
   IMeshPtr generateBezierRailMesh(const BezierCurve<float>& func)
   {
      IMeshBufferPtr buf = makeMeshBuffer();
      
      buildOneBezierRail(func, buf, GAUGE/2.0f);
      buildOneBezierRail(func, buf, -GAUGE/2.0f);

      return makeMesh(buf);
   }
   
   enum RailType {
      INNER_RAIL, OUTER_RAIL
   };
   
   IMeshPtr generateCurvedRailMesh(IMeshBufferPtr buf, int baseRadius, RailType type)
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

      return makeMesh(buf);
   }
   
   void renderCurvedRail(int baseRadius)
   {
      IMeshPtr ptr;
      
      CurvedRailMeshMap::iterator it = theCurvedRailMeshes.find(baseRadius);
      if (it != theCurvedRailMeshes.end())
         ptr = (*it).second;
      else {
         IMeshBufferPtr buf = makeMeshBuffer();
         
         generateCurvedRailMesh(buf, baseRadius, INNER_RAIL);
         generateCurvedRailMesh(buf, baseRadius, OUTER_RAIL);
            
         ptr = makeMesh(buf);
         theCurvedRailMeshes[baseRadius] = ptr;
      }

      ptr->render();
   }

   void renderOneRail()
   {
      if (!theRailMesh)
         generateRailMesh();
      
      theRailMesh->render();
   }

}

IMeshPtr makeBezierRailMesh(const BezierCurve<float>& aFunc)
{
   return generateBezierRailMesh(aFunc);
}

IMeshBufferPtr SleeperHelper::sleeperBuf;

void SleeperHelper::mergeSleeper(IMeshBufferPtr buf,
   Vector<float> off, float yAngle) const
{
   if (!sleeperBuf)
      sleeperBuf = generateSleeperMeshBuffer();

   buf->merge(sleeperBuf, off, yAngle);
}

IMeshBufferPtr BezierHelper::makeBezierRailMesh(
   const BezierCurve<float>& func) const
{
   IMeshBufferPtr buf = makeMeshBuffer();
   
   buildOneBezierRail(func, buf, GAUGE/2.0f);
   buildOneBezierRail(func, buf, -GAUGE/2.0f);

   return buf;
}

// Draw a sleeper in the current matrix location
void renderSleeper()
{   
   if (!theSleeperMesh)
      generateSleeperMesh();
   
   theSleeperMesh->render();
}

// Render a pre-generated rail mesh in the right place
void renderRailMesh(IMeshPtr aMesh)
{
   glPushMatrix();

   glTranslatef(-0.5f, 0.0f, -GAUGE/2.0f);   
   aMesh->render();
   
   glTranslatef(0.0f, 0.0f, GAUGE);
   aMesh->render();

   glPopMatrix();
}

void renderStraightRail()
{
   glPushMatrix();

   glTranslatef(-GAUGE/2.0f, 0.0f, -0.5f);
   renderOneRail();
   
   glTranslatef(GAUGE, 0.0f, 0.0f);
   renderOneRail();

   glPopMatrix();
}

IMeshBufferPtr StraightTrackHelper::railBuf;

void StraightTrackHelper::mergeOneRail(IMeshBufferPtr buf,
   Vector<float> off, float yAngle) const
{
   if (!railBuf)
      railBuf= generateRailMeshBuffer();
   
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

// Move to the origin of a curved section of track
void transformToOrigin(int baseRadius, track::Angle startAngle)
{
   glTranslatef((baseRadius-1)*-sin(degToRad(startAngle)) - 0.5f, 0.0f,
      (baseRadius-1)*-cos(degToRad(startAngle)) - 0.5f);

   // There *must* be a way to incorporate this in the above translation
   // as a neat formula, but I really can't think of it
   // This is a complete a hack, but whatever...
   if (startAngle >= 90 && startAngle <= 180)
      glTranslatef(0.0f, 0.0f, 1.0f);
   
   if (startAngle >= 180 && startAngle <= 270)
      glTranslatef(1.0f, 0.0f, 0.0f);
}

// `baseRadius' is measured in tiles
void renderCurvedTrack(int baseRadius, track::Angle startAngle,
   track::Angle endAngle)
{
   glPushMatrix();
   
   transformToOrigin(baseRadius, startAngle);

   glPushMatrix();
   
   glRotatef(static_cast<float>(startAngle), 0.0f, 1.0f, 0.0f);
   renderCurvedRail(baseRadius);

   glPopMatrix();

   const float length = degToRad(static_cast<float>(endAngle - startAngle)) * baseRadius;
   const int numSleepers = static_cast<int>(length * SLEEPERS_PER_UNIT);
   const float sleeperAngle =
      static_cast<float>(endAngle - startAngle) / numSleepers;
   
   for (int i = 0; i < numSleepers; i++) {
      glPushMatrix();
      
      glRotatef(static_cast<float>(startAngle) + (i + 0.5f)*sleeperAngle,
         0.0f, 1.0f, 0.0f);
      glTranslatef(0.0f, 0.0f, static_cast<float>(baseRadius) - 0.5f);
      
      renderSleeper();
      
      glPopMatrix();
   }

   glPopMatrix();
}
