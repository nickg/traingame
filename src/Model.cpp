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
#include "ILogger.hpp"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <cassert>
#include <stdexcept>

#include <GL/gl.h>

using namespace std;

// A model contains the display list to render it
class Model : public IModel {
public:
   Model(GLuint aDisplayList) : myDisplayList(aDisplayList) {}
   ~Model();
   
   void render() const;
private:
   GLuint myDisplayList;
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
IModelPtr loadModel(const string& fileName)
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

   GLenum displayList = glGenLists(1);
   glNewList(displayList, GL_COMPILE);

   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glPushMatrix();
   
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);
   glEnable(GL_TEXTURE);
   glEnable(GL_LIGHTING);

   glColor3d(1.0, 1.0, 1.0);
   
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

         vertices.clear();
         normals.clear();
         textureOffs.clear();
      }
      else if (first == "mtllib") {
         // Material file
      }
      else if (first == "v") {
         // Vertex
         double x, y, z;
         f >> x >> y >> z;

         vertices.push_back(makeVector(x, y, z));
      }
      else if (first == "vn") {
         // Normal
         double x, y, z;
         f >> x >> y >> z;
         
         normals.push_back(makeVector(x, y, z));
      }
      else if (first == "g" || first == "usemtl") {
         // ???
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
            ss >> vi >> delim1 >> vti >> delim2 >> vni;
            assert(delim1 == '/' && delim2 == '/');

            Vector<double>& vn = normals[vni - 1];
            glNormal3d(vn.x, vn.y, vn.z);

            if (vti - 1 < textureOffs.size()) {
               Point<double>& vt = textureOffs[vti - 1];
               glTexCoord2d(vt.x, vt.y);
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

   glPopMatrix();
   glPopAttrib();
   glEndList();
   
   return IModelPtr(new Model(displayList));
}
