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

#include "Maths.hpp"
#include "IModel.hpp"
#include "ITextureManager.hpp"

#include <cstdio>
#include <cstring>
#include <string>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <vector>

#include <GL/gl.h>

using namespace std;

// A face of the model
struct Face {
   static const int MAX_VERTEX = 16;
   
   struct VertexDesc {
      unsigned short v, n, t;   // Vertex, normal and texture indicies
   } vdesc[MAX_VERTEX];         // Maximum of 16 verticies in a face...
   
   int vertexCount;       // The number of verticies (in this face)
};

typedef vector<Face> FaceList;
typedef vector<Vector<double> > VectorList;
typedef vector<Point<double> > PointList;

// The model itself
struct Model : IModel {   
   void render();
   
   FaceList   faces;           // The model's faces
   VectorList verticies;       // The model's verticies
   VectorList normals;         // The model's normals
   PointList  texCoords;       // The model's texture coordinates
   GLuint     uTexture;        // Model texture map
   int        faceCount;       // Total number of faces
   float      zMin, zMax;      // Minimum and maximum Z-coordinates
   float      xMin, xMax;      // Minimum and maximum X-coordinates
   float      yMin, yMax;      // Minimum and maximum Y-coordinates

   ITexturePtr texture;
};

// Load a WaveFront .obj model from disk
IModelPtr loadModel(const string& fileName)
{
   auto_ptr<Model> pModel = auto_ptr<Model>(new Model);

   pModel->texture = getTextureManager()->load("/home/nick/cube1_auv.bmp");

   char ch, line[256];
   int vertexCount=0, normalCount=0, texCoordCount=0, faceCount=0;
   int texCoordIterator=0, normalIterator=0, faceIterator=0, vertexIterator=0;
   
   pModel->xMin = pModel->xMax = pModel->yMin
      = pModel->yMax = pModel->zMin = pModel->zMax = 0.0f;
    
   const char* p = fileName.c_str();
   FILE* f = fopen(p, "r");
   if (f == NULL) {
      ostringstream ss;
      ss << "Cannot open model file: " << fileName;
      throw runtime_error(ss.str());
   }
   
   // First pass, count up number of vertices, texture coords and normals
   while (!feof(f)) {
      fscanf(f, "%s", line);
      if (strcasecmp(line, "v") == 0)
         vertexCount++;
      else if (strcasecmp(line, "vn") == 0)
         normalCount++;
      else if (strcasecmp(line, "vt") == 0)
         texCoordCount++;
      else if (strcasecmp(line, "f") == 0)
         faceCount++;
      fgets(line, 256, f);
   }

   // Allocate memory
   pModel->faceCount = faceCount;
   pModel->faces.reserve(faceCount);
   pModel->verticies.reserve(vertexCount);
   pModel->normals.reserve(normalCount);
   pModel->texCoords.reserve(texCoordCount);
   
   rewind(f);

   // Loop through each line
   float x, y, z;
   while (!feof(f)) {
      ch = fgetc(f);
      if (ch == 'v') {
         // Some sort of vertex
         char next = fgetc(f);
         if (next == 'n') {
            // Its a normal
            fscanf(f, "%f %f %f", &x, &y, &z);
            pModel->normals[normalIterator].x = x;
            pModel->normals[normalIterator].y = y;
            pModel->normals[normalIterator].z = z;
            normalIterator++;
         }
         else if (next == 't') {
            // Its a texture coordinate
            fscanf(f, "%f %f", &x, &y);
            pModel->texCoords[texCoordIterator].x = x;
            pModel->texCoords[texCoordIterator].y = y;
            texCoordIterator++;
         }
         else if (next == ' ') {
            // Its a vertex
            fscanf(f, "%f %f %f", &x, &y, &z);
            pModel->verticies[vertexIterator].x = x;
            pModel->verticies[vertexIterator].y = y;
            pModel->verticies[vertexIterator].z = z;
            
            // Check size
            if (x < pModel->xMin) pModel->xMin = x;
            else if (x > pModel->xMax) pModel->xMax = x;
            if (y < pModel->yMin) pModel->yMin = y;
            else if (y > pModel->yMax) pModel->yMax = y;
            if (z < pModel->zMin) pModel->zMin = z;
            else if (z > pModel->zMax) pModel->zMax = z;
            
            vertexIterator++;
         }
         fgets(line, 256, f);
      }
      else if (ch == '#') {
         // Its a comment so ignore it
         fgets(line, 256, f);
      }
      else if (ch == 'f') {
         // Its a face
         // The normal indicies are ignored (is this ok???)
         int v[16], vt[16], vn[16];
         int read = 0;
         fgets(line, 256, f);
         char *token = strtok(line, " \n");
         while (token != NULL) {
            sscanf(token, "%d/%d/%d", &v[read], &vt[read], &vn[read]);
            read++;
            token = strtok(NULL, " \n");
         }
         if (read > Face::MAX_VERTEX)
            throw runtime_error("Too many verticies in model face");
         else
            pModel->faces[faceIterator].vertexCount = read;
         for (int i = 0; i < read; i++) {
            pModel->faces[faceIterator].vdesc[i].v = v[i]-1;
            pModel->faces[faceIterator].vdesc[i].n = vn[i]-1;
            pModel->faces[faceIterator].vdesc[i].t = vt[i]-1;
         }
         faceIterator++;
      }
   }
   
   return IModelPtr(pModel);
}

// Render the 3D model in the current view
void Model::render()
{
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);
   glEnable(GL_TEXTURE);
   glEnable(GL_LIGHTING);
   texture->bind();
   glColor3f(1.0f, 1.0f, 1.0f);
   for (int i = 0; i < faceCount; i++) {
      switch(faces[i].vertexCount) {
      case 3: glBegin(GL_TRIANGLES); break;
      case 4: glBegin(GL_QUADS); break;
      default: glBegin(GL_POLYGON); break;
      }
      for (int j = 0; j < faces[i].vertexCount; j++) {
         int v = faces[i].vdesc[j].v;
         int n = faces[i].vdesc[j].n;
         int t = faces[i].vdesc[j].t;
         glNormal3f(normals[n].x, normals[n].y, normals[n].z);
         glTexCoord2f(texCoords[t].x, 1.0f - texCoords[t].y);
         glVertex3f(verticies[v].x, verticies[v].y, verticies[v].z);
      }
      glEnd();
   }
}

