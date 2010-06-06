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

#include "Platform.hpp"
#include "Maths.hpp"

#include <GL/gl.h>

using namespace std;

/* The sides of the frustum */
enum FrustumSide { RIGHT=0, LEFT, BOTTOM, TOP, BACK, FRONT };

/* Coefficients of plane equation */
enum PlaneData {
   A = 0,				// The X value of the plane's normal
   B = 1,				// The Y value of the plane's normal
   C = 2,				// The Z value of the plane's normal
   D = 3				// The distance the plane is from the origin
};

// Normalise a plane's normal vector
static void normalisePlane(float frustum[6][4], int side)
{
   float magnitude = (float)sqrtf(frustum[side][A]*frustum[side][A] + 
      frustum[side][B]*frustum[side][B] + 
      frustum[side][C]*frustum[side][C]);
   
   frustum[side][A] /= magnitude;
   frustum[side][B] /= magnitude;
   frustum[side][C] /= magnitude;
   frustum[side][D] /= magnitude; 
}

// Tests whether a point is in the frustum or not
bool Frustum::pointInFrustum(float x, float y, float z) 
{
   // Look through each plane...
   for (int i = 0; i < 6; i++) {
      if(planes[i][A]*x + planes[i][B]*y + planes[i][C]*z + planes[i][D] < 0)
         return false;
   }
   
   return true;
}

// Tests whether a sphere is inside the frustum or not
bool Frustum::sphereInFrustum(float x, float y, float z, float radius)
{
   // Look through each plane
   for (int i = 0; i < 6; i++) {
      if (planes[i][A]*x + planes[i][B]*y + planes[i][C]*z + planes[i][D] <= -radius)
         return false;
   }

   return true;
}

// Tests whether a cube is in the view frustum 
bool Frustum::cubeInFrustum(float x, float y, float z, float size)
{
   // Look through each plane
   for (int i = 0; i < 6; i++ ) {
      if (planes[i][A]*(x - size) + planes[i][B]*(y - size) + planes[i][C]*(z - size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x + size) + planes[i][B]*(y - size) + planes[i][C]*(z - size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x - size) + planes[i][B]*(y + size) + planes[i][C]*(z - size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x + size) + planes[i][B]*(y + size) + planes[i][C]*(z - size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x - size) + planes[i][B]*(y - size) + planes[i][C]*(z + size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x + size) + planes[i][B]*(y - size) + planes[i][C]*(z + size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x - size) + planes[i][B]*(y + size) + planes[i][C]*(z + size) + planes[i][D] > 0.0)
         continue;
      if(planes[i][A]*(x + size) + planes[i][B]*(y + size) + planes[i][C]*(z + size) + planes[i][D] > 0.0)
         continue;

      // If we get here, it isn't in the frustum
      return false;
   }

   return true;
}

// Works out whether a cuboid is contained in a frustum
bool Frustum::cuboidInFrustum(float x,	  float y,	   float z,
   float sizeX, float sizeY, float sizeZ)
{
   int i;

   // Look through each plane
   for(i = 0; i < 6; i++ )
      {
         if(planes[i][A]*(x - sizeX) + planes[i][B]*(y - sizeY) + planes[i][C]*(z - sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x + sizeX) + planes[i][B]*(y - sizeY) + planes[i][C]*(z - sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x - sizeX) + planes[i][B]*(y + sizeY) + planes[i][C]*(z - sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x + sizeX) + planes[i][B]*(y + sizeY) + planes[i][C]*(z - sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x - sizeX) + planes[i][B]*(y - sizeY) + planes[i][C]*(z + sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x + sizeX) + planes[i][B]*(y - sizeY) + planes[i][C]*(z + sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x - sizeX) + planes[i][B]*(y + sizeY) + planes[i][C]*(z + sizeZ) + planes[i][D] > 0)
            continue;
         if(planes[i][A]*(x + sizeX) + planes[i][B]*(y + sizeY) + planes[i][C]*(z + sizeZ) + planes[i][D] > 0)
            continue;

         // If we get here, it isn't in the frustum
         return false;
      }

   return true;
}

// Extract the view frustum from OpenGL
Frustum getViewFrustum()
{
   float proj[16];
   float modl[16];
   float clip[16];

   // Extract projection matrix
   glGetFloatv(GL_PROJECTION_MATRIX, proj);

   // Extract modelview matrix
   glGetFloatv(GL_MODELVIEW_MATRIX, modl);
   
   // Multiply both matricies to get clipping planes
   clip[ 0] = modl[ 0]*proj[ 0] + modl[ 1]*proj[ 4] + modl[ 2]*proj[ 8] + modl[ 3]*proj[12];
   clip[ 1] = modl[ 0]*proj[ 1] + modl[ 1]*proj[ 5] + modl[ 2]*proj[ 9] + modl[ 3]*proj[13];
   clip[ 2] = modl[ 0]*proj[ 2] + modl[ 1]*proj[ 6] + modl[ 2]*proj[10] + modl[ 3]*proj[14];
   clip[ 3] = modl[ 0]*proj[ 3] + modl[ 1]*proj[ 7] + modl[ 2]*proj[11] + modl[ 3]*proj[15];
   
   clip[ 4] = modl[ 4]*proj[ 0] + modl[ 5]*proj[ 4] + modl[ 6]*proj[ 8] + modl[ 7]*proj[12];
   clip[ 5] = modl[ 4]*proj[ 1] + modl[ 5]*proj[ 5] + modl[ 6]*proj[ 9] + modl[ 7]*proj[13];
   clip[ 6] = modl[ 4]*proj[ 2] + modl[ 5]*proj[ 6] + modl[ 6]*proj[10] + modl[ 7]*proj[14];
   clip[ 7] = modl[ 4]*proj[ 3] + modl[ 5]*proj[ 7] + modl[ 6]*proj[11] + modl[ 7]*proj[15];

   clip[ 8] = modl[ 8]*proj[ 0] + modl[ 9]*proj[ 4] + modl[10]*proj[ 8] + modl[11]*proj[12];
   clip[ 9] = modl[ 8]*proj[ 1] + modl[ 9]*proj[ 5] + modl[10]*proj[ 9] + modl[11]*proj[13];
   clip[10] = modl[ 8]*proj[ 2] + modl[ 9]*proj[ 6] + modl[10]*proj[10] + modl[11]*proj[14];
   clip[11] = modl[ 8]*proj[ 3] + modl[ 9]*proj[ 7] + modl[10]*proj[11] + modl[11]*proj[15];
   
   clip[12] = modl[12]*proj[ 0] + modl[13]*proj[ 4] + modl[14]*proj[ 8] + modl[15]*proj[12];
   clip[13] = modl[12]*proj[ 1] + modl[13]*proj[ 5] + modl[14]*proj[ 9] + modl[15]*proj[13];
   clip[14] = modl[12]*proj[ 2] + modl[13]*proj[ 6] + modl[14]*proj[10] + modl[15]*proj[14];
   clip[15] = modl[12]*proj[ 3] + modl[13]*proj[ 7] + modl[14]*proj[11] + modl[15]*proj[15];

   Frustum f;
	
   // Extract RIGHT plane
   f.planes[RIGHT][A] = clip[ 3] - clip[ 0];
   f.planes[RIGHT][B] = clip[ 7] - clip[ 4];
   f.planes[RIGHT][C] = clip[11] - clip[ 8];
   f.planes[RIGHT][D] = clip[15] - clip[12];

   // Normalize the RIGHT plane
   normalisePlane(f.planes, RIGHT);

   // Extract the LEFT plane
   f.planes[LEFT][A] = clip[ 3] + clip[ 0];
   f.planes[LEFT][B] = clip[ 7] + clip[ 4];
   f.planes[LEFT][C] = clip[11] + clip[ 8];
   f.planes[LEFT][D] = clip[15] + clip[12];

   // Normalize the LEFT plane
   normalisePlane(f.planes, LEFT);

   // Extract the BOTTOM plane
   f.planes[BOTTOM][A] = clip[ 3] + clip[ 1];
   f.planes[BOTTOM][B] = clip[ 7] + clip[ 5];
   f.planes[BOTTOM][C] = clip[11] + clip[ 9];
   f.planes[BOTTOM][D] = clip[15] + clip[13];
   
   // Normalize the BOTTOM plane
   normalisePlane(f.planes, BOTTOM);
   
   // Extract TOP plane
   f.planes[TOP][A] = clip[ 3] - clip[ 1];
   f.planes[TOP][B] = clip[ 7] - clip[ 5];
   f.planes[TOP][C] = clip[11] - clip[ 9];
   f.planes[TOP][D] = clip[15] - clip[13];
   
   // Normalize the TOP plane
   normalisePlane(f.planes, TOP);
   
   // Extract the BACK plane
   f.planes[BACK][A] = clip[ 3] - clip[ 2];
   f.planes[BACK][B] = clip[ 7] - clip[ 6];
   f.planes[BACK][C] = clip[11] - clip[10];
   f.planes[BACK][D] = clip[15] - clip[14];
   
   // Normalize the BACK plane
   normalisePlane(f.planes, BACK);
   
   // Extract the FRONT plane
   f.planes[FRONT][A] = clip[ 3] + clip[ 2];
   f.planes[FRONT][B] = clip[ 7] + clip[ 6];
   f.planes[FRONT][C] = clip[11] + clip[10];
   f.planes[FRONT][D] = clip[15] + clip[14];

   // Normalize the FRONT plane
   normalisePlane(f.planes, FRONT);

   return f;
}
