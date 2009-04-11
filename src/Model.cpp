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
#include "ITexture.hpp"
#include "ILogger.hpp"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <algorithm>

#include <GL/gl.h>

using namespace std;

// A model contains the display list to render it
class Model : public IModel {
public:
   Model(GLuint aDisplayList, const Vector<double>& aDim)
      : myDisplayList(aDisplayList), myDimensions(aDim) {}
   ~Model();
   
   void render() const;
   Vector<double> dimensions() const { return myDimensions; }
private:
   GLuint myDisplayList;
   Vector<double> myDimensions;
};

// Free the display list
Model::~Model()
{
   glDeleteLists(myDisplayList, 1);
}

void Model::render() const
{
   glCallList(myDisplayList);
}

// Load a WaveFront .obj model from disk
// Each vertex is scaled by `aScale'
IModelPtr loadModel(const string& fileName, double aScale)
{
   ifstream f(fileName.c_str());
   if (!f.good()) {
      ostringstream ss;
      ss << "Failed to open model: " << fileName;
      throw runtime_error(ss.str());
   }

   log() << "Loading model " << fileName;

   vector<Vector<double> > vertices, normals;
   vector<Point<double> > textureOffs;

   ITexturePtr texture;

   GLenum displayList = glGenLists(1);
   glNewList(displayList, GL_COMPILE);
   
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glPushMatrix();
   
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);
   glEnable(GL_TEXTURE);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_CULL_FACE);

   glColor4d(1.0, 1.0, 1.0, 1.0);

   bool foundVertex = false;
   double ymin = 0, ymax = 0, xmin = 0, xmax = 0,
      zmin = 0, zmax = 0;
   
   while (!f.eof()) {
      string first;
      f >> first;

      if (first[0] == '#') {
         // Comment
      }
      else if (first == "o") {
         // New object
         string objName;
         f >> objName;
         debug() << "Building object " << objName;

         texture.reset();
      }
      else if (first == "mtllib") {
         // Material file
      }
      else if (first == "v") {
         // Vertex
         double x, y, z;
         f >> x >> y >> z;

         x *= aScale;
         y *= aScale;
         z *= aScale;

         if (foundVertex) {
            xmin = min(x, xmin);
            xmax = max(x, xmax);

            ymin = min(y, ymin);
            ymax = max(y, ymax);

            zmin = min(z, zmin);
            zmax = max(z, zmax);
         }
         else {
            xmin = xmax = x;
            ymin = ymax = y;
            zmin = zmax = z;

            foundVertex = true;
         }

         vertices.push_back(makeVector(x, y, z));
      }
      else if (first == "vn") {
         // Normal
         double x, y, z;
         f >> x >> y >> z;
         
         normals.push_back(makeVector(x, y, z));
      }
      else if (first == "vt") {
         // Texture coordinate
         double x, y;
         f >> x >> y;

         textureOffs.push_back(makePoint(x, y));
      }
      else if (first == "g") {
         // Ignore it (texture file comes from material name)
      }
      else if (first == "usemtl") {
         // Set the material for this object
         string texBaseName;
         f >> texBaseName;

         bool exists = ifstream((texBaseName + ".bmp").c_str()).good();

         if (exists) 
            texture = loadTexture(texBaseName + ".bmp");
         else
            log() << "No texture for material " << texBaseName;
      }
      else if (first == "f") {
         // Face
         string line;
         getline(f, line);
         istringstream ss(line);

         glBegin(GL_POLYGON);
         while (!ss.eof()) {
            char delim1, delim2;
            unsigned vi, vti, vni;
            ss >> vi >> delim1;

            // Texture coordinate may be omitted
            ss >> vti;
            if (ss.fail()) {
               vti = -1;
               ss.clear();
            }

            ss >> delim2 >> vni;
            assert(delim1 == '/' && delim2 == '/');

            Vector<double>& vn = normals[vni - 1];
            glNormal3d(vn.x, vn.y, vn.z);

            if (vti - 1 < textureOffs.size()) {
               Point<double>& vt = textureOffs[vti - 1];
               glTexCoord2d(vt.x, 1.0 - vt.y);
            }

            Vector<double>& v = vertices[vi - 1];
            glVertex3d(v.x, v.y, v.z);
         }
         glEnd();
            
         // Don't discard the next line
         continue;
      }

      // Discard the rest of the line
      getline(f, first);
   }

   Vector<double> dim = makeVector(xmax - xmin, ymax - ymin, zmax - zmin);
   log() << dim;

   log() << "Model loaded: " << vertices.size() << " vertices";
   
   glPopMatrix();
   glPopAttrib();
   glEndList();
   
   return IModelPtr(new Model(displayList, dim));
}
