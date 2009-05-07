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
#include <map>

#include <boost/lexical_cast.hpp>

#include <GL/gl.h>

using namespace std;
using namespace std::tr1;
using namespace boost;

// Cache of already loaded models
namespace {
   typedef map<string, IModelPtr> ModelCache;
   ModelCache theCache;
}

// Abstracts a WaveFront material file
class MaterialFile {
public:
   MaterialFile(const string& aFileName);
   ~MaterialFile() {}

   void apply(const string& aName) const;
private:
   struct Material {
      float diffuseR, diffuseG, diffuseB;
      float ambientR, ambientG, ambientB;
      float specularR, specularG, specularB;
      ITexturePtr texture;
   };

   typedef map<string, Material> MaterialSet;
   MaterialSet myMaterials;
};

typedef shared_ptr<MaterialFile> MaterialFilePtr;

MaterialFile::MaterialFile(const string& aFileName)
{
   ifstream is(aFileName.c_str());
   if (!is.good())
      throw runtime_error("Failed to load material: " + aFileName);

   log() << "Loading materials from " << aFileName;

   string activeMaterial;
   while (!is.eof()) {
      string word;
      is >> word;

      if (word[0] == '#') {
         // Comment
         continue;
      }
      else if (word == "newmtl") {
         is >> activeMaterial;
         debug() << "Loading material " << activeMaterial;

         Material m = { 0, 0, 0,    // Diffuse
                        0, 0, 0,    // Ambient
                        0, 0, 0 };  // Specular
         myMaterials[activeMaterial] = m;
      }
      else if (word == "map_Kd") {
         // Texture
         is >> word;
         myMaterials[activeMaterial].texture = loadTexture(word);
      }
      else if (word == "Kd") {
         // Diffuse colour
         Material& m = myMaterials[activeMaterial];
         is >> m.diffuseR >> m.diffuseG >> m.diffuseB;
      }
      else if (word == "Ka") {
         // Ambient colour
         Material& m = myMaterials[activeMaterial];
         is >> m.ambientR >> m.ambientG >> m.ambientB;
      }
      else if (word == "Ks") {
         // Specular colour
         Material& m = myMaterials[activeMaterial];
         is >> m.specularR >> m.specularG >> m.specularB;
      }
      else {
         // Ignore it
         continue;
      }
   }
}

void MaterialFile::apply(const string& aName) const
{
   MaterialSet::const_iterator it = myMaterials.find(aName);
   if (it == myMaterials.end())
      throw runtime_error("No material named " + aName);

   const Material& m = (*it).second;

   if (m.texture) {
      m.texture->bind();
      glEnable(GL_TEXTURE_2D);
   }
   else
      glDisable(GL_TEXTURE_2D);

   glDisable(GL_COLOR_MATERIAL);

   float diffuse[] = { m.diffuseR, m.diffuseG, m.diffuseB, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);

   float ambient[] = { m.ambientR, m.ambientG, m.ambientB, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);

   // Note we're ignoring the specular values in the model
   float specular[] = { 0, 0, 0, 1.0 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);

   float emission[] = { 0, 0, 0, 1 };
   glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
}

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

// Load a WaveFront .obj model from disk or the cache
// Each vertex is scaled by `aScale'
IModelPtr loadModel(const string& fileName, double aScale)
{
   ModelCache::iterator it = theCache.find(fileName);
   if (it != theCache.end()) {
      debug() << "Got model " << fileName << " from cache";
      return (*it).second;
   }
   
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
   
   glPushAttrib(GL_ENABLE_BIT);
   glPushMatrix();
   
   glEnable(GL_DEPTH_TEST);
   glDisable(GL_BLEND);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_CULL_FACE);

   bool foundVertex = false;
   double ymin = 0, ymax = 0, xmin = 0, xmax = 0,
      zmin = 0, zmax = 0;
   int faceCount = 0;

   MaterialFilePtr materialFile;
   string materialName;
   
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
      }
      else if (first == "mtllib") {
         // Material file
         string fileName;
         f >> fileName;
         
         materialFile = MaterialFilePtr(new MaterialFile(fileName));
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
         f >> materialName;
      }
      else if (first == "f") {
         // Face
         string line;
         getline(f, line);
         istringstream ss(line);

         if (materialFile)
            materialFile->apply(materialName);

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

         faceCount++;
            
         // Don't discard the next line
         continue;
      }

      // Discard the rest of the line
      getline(f, first);
   }

   Vector<double> dim = makeVector(xmax - xmin, ymax - ymin, zmax - zmin);
   log() << dim;

   log() << "Model loaded: " << vertices.size() << " vertices, "
         << faceCount << " faces";
   
   glPopMatrix();
   glPopAttrib();
   glEndList();
   
   IModelPtr ptr(new Model(displayList, dim));
   
   theCache[fileName] = ptr;
   
   return ptr;
}
