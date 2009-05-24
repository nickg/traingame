//
//  Copyright (C) 2009  Nick Gasson
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

#include <cmath>
#include <map>

#include <GL/gl.h>

using namespace std;

namespace {
   const float RAIL_WIDTH = 0.05f;
   const float GAUGE = 0.5f;

   const int SLEEPERS_PER_UNIT = 4;

   const float SLEEPER_LENGTH = 0.8;

   IMeshPtr theSleeperMesh, theRailMesh;

   typedef map<int, IMeshPtr> CurvedRailMeshMap;
   CurvedRailMeshMap theCurvedRailMeshes;
   
   void generateSleeperMesh()
   {
      IMeshBufferPtr buf = makeMeshBuffer();

      const IMeshBuffer::Colour brown = make_tuple(0.5f, 0.3f, 0.0f);

      const float sleeperWidth = 0.1f;
      const float sleeperDepth = 0.05f;
      const float sleeperOff = sleeperWidth / 2.0;

      const float r = SLEEPER_LENGTH / 2.0;

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
   
      theSleeperMesh = makeMesh(buf);
   }

   void generateRailMesh()
   {
      IMeshBufferPtr buf = makeMeshBuffer();

      const IMeshBuffer::Colour metal = make_tuple(0.7f, 0.7f, 0.7f);
      
      // Top side
      buf->addQuad(makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
                   makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
                   makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
                   makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
                   metal);
      
      // Outer side
      buf->addQuad(makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
                   makeVector(-RAIL_WIDTH/2.0f, 0.0f, 0.0f),
                   makeVector(-RAIL_WIDTH/2.0f, 0.0f, 1.0f),
                   makeVector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
                   metal);
   
      // Inner side
      buf->addQuad(makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
                   makeVector(RAIL_WIDTH/2.0f, 0.0f, 1.0f),
                   makeVector(RAIL_WIDTH/2.0f, 0.0f, 0.0f),
                   makeVector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
                   metal);   
      
      theRailMesh = makeMesh(buf);
   }

   enum RailType {
      INNER_RAIL, OUTER_RAIL
   };
   
   IMeshPtr generateCurvedRailMesh(IMeshBufferPtr buf, int baseRadius, RailType type)
   {
      const float edgeWidth = (1 - GAUGE - RAIL_WIDTH)/2.0;
      const float R = static_cast<float>(baseRadius) - edgeWidth
      - (type == OUTER_RAIL ? 0 : GAUGE);
      const float r = R - RAIL_WIDTH;
      
      const float step = 0.1f;

      const IMeshBuffer::Colour metal = make_tuple(0.7f, 0.7f, 0.7f);
      
      // Top of rail
      for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
         buf->addQuad(makeVector(r * cos(theta), 0.1f, r * sin(theta)), 
                      makeVector(r * cos(theta + step), 0.1f, r * sin(theta + step)),
                      makeVector(R * cos(theta + step), 0.1f, R * sin(theta + step)),
                      makeVector(R * cos(theta), 0.1f, R * sin(theta)),
                      metal);
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

                      metal);
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

                      metal);
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

// Draw a sleeper in the current maxtrix location
void renderSleeper()
{
   if (!theSleeperMesh)
      generateSleeperMesh();
   
   theSleeperMesh->render();
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

   const float length = degToRad(endAngle - startAngle) * baseRadius;
   const int numSleepers = length * SLEEPERS_PER_UNIT;
   const float sleeperAngle =
      static_cast<float>(endAngle - startAngle) / numSleepers;
   
   for (int i = 0; i < numSleepers; i++) {
      glPushMatrix();
      
      glRotatef(static_cast<float>(startAngle) + (i + 0.5)*sleeperAngle,
                0.0f, 1.0f, 0.0f);
      glTranslatef(0.0f, 0.0f, static_cast<float>(baseRadius) - 0.5f);
      
      renderSleeper();
      
      glPopMatrix();
   }

   glPopMatrix();
}
