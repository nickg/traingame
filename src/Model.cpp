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
#include "IMesh.hpp"
#include "ResourceCache.hpp"

#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <cassert>
#include <stdexcept>
#include <algorithm>
#include <map>
#include <list>

#include <boost/lexical_cast.hpp>

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
   MaterialFile(const string& aFileName, IResourcePtr aRes);
   ~MaterialFile() {}

   const Material& get(const string& aName) const;
private:
   typedef map<string, Material> MaterialSet;
   MaterialSet myMaterials;
};

typedef shared_ptr<MaterialFile> MaterialFilePtr;

MaterialFile::MaterialFile(const string& aFileName, IResourcePtr aRes)
{
   IResource::Handle h = aRes->openFile(aFileName);
   
   log() << "Loading materials from " << h.fileName();

   istream& is = h.rstream();
   
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

         myMaterials[activeMaterial] = Material();
      }
      else if (word == "map_Kd") {
         // Texture
         is >> word;
         myMaterials[activeMaterial].texture = loadTexture(aRes, word);
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

const Material& MaterialFile::get(const string& aName) const
{
   MaterialSet::const_iterator it = myMaterials.find(aName);
   if (it == myMaterials.end())
      throw runtime_error("No material named " + aName);

   return (*it).second;
}

class Model : public IModel {
public:
   Model(const Vector<float>& dim, const IMeshBufferPtr buf)
      : dimensions_(dim), buffer(buf)
   {}
   ~Model();
   
   void render() const;
   void cache();
   Vector<float> dimensions() const { return dimensions_; }
private:
   void compileMesh() const;
   
   Vector<float> dimensions_;
   mutable IMeshPtr mesh;
   const IMeshBufferPtr buffer;
};

Model::~Model()
{
   
}

void Model::cache()
{
   if (!mesh)
      compileMesh();
}

void Model::render() const
{
   if (!mesh)
      compileMesh();
   
   mesh->render();
}

void Model::compileMesh() const
{
   // Const as may be called during render
   mesh = makeMesh(buffer);
}

// Load a model from a resource
IModelPtr loadModel(IResourcePtr aRes, const string& aFileName, float aScale)
{
   // Make a unique cache name
   const string cacheName = aRes->name() + ":" + aFileName;

   // Check the cache for the model
   ModelCache::iterator it = theCache.find(cacheName);
   if (it != theCache.end())
      return (*it).second;

   // Not in the cache, load it from the resource
   IResource::Handle h = aRes->openFile(aFileName);
   log() << "Loading model " << h.fileName();

   vector<IMeshBuffer::Vertex> vertices;
   vector<IMeshBuffer::Normal> normals;
   vector<IMeshBuffer::TexCoord> textureOffs;

   IMeshBufferPtr buffer = makeMeshBuffer();

   bool foundVertex = false;
   float ymin = 0, ymax = 0, xmin = 0, xmax = 0,
      zmin = 0, zmax = 0;
   int faceCount = 0;

   MaterialFilePtr materialFile;

   ifstream& f = h.rstream();
   
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
      }
      else if (first == "mtllib") {
         // Material file
         string fileName;
         f >> fileName;
         
         materialFile =
            MaterialFilePtr(new MaterialFile(fileName, aRes));
      }
      else if (first == "v") {
         // Vertex
         float x, y, z;
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
         float x, y, z;
         f >> x >> y >> z;
         
         normals.push_back(makeVector(x, y, z));
      }
      else if (first == "vt") {
         // Texture coordinate
         float x, y;
         f >> x >> y;

         textureOffs.push_back(makePoint(x, y));
      }
      else if (first == "g") {
         // Groups used to correspond to sub-meshes but now
         // the whole model is compiled into a single mesh
      }
      else if (first == "usemtl") {
         // Set the material for this group
         string materialName;
         f >> materialName;
         
         if (materialFile) {
            assert(buffer);
            buffer->bindMaterial(materialFile->get(materialName));
         }
      }
      else if (first == "f") {
         // Face
         string line;
         getline(f, line);
         istringstream ss(line);

         int vInThisFace = 0;
         
         while (!ss.eof()) {
            char delim1, delim2;
            unsigned vi, vti, vni;
            ss >> vi >> delim1;
            if (ss.fail())
               break;

            if (++vInThisFace > 3)
               warn () << "All model faces must be triangles "
                       << "(face with " << vInThisFace << " vertices)";

            // Texture coordinate may be omitted
            ss >> vti;
            if (ss.fail()) {
               vti = -1;
               ss.clear();
            }

            ss >> delim2 >> vni;
            assert(delim1 == '/' && delim2 == '/');

            Vector<float>& v = vertices[vi - 1];
            Vector<float>& vn = normals[vni - 1];

            assert(buffer);
            
            if (vti - 1 < textureOffs.size()) {
               Point<float>& vt = textureOffs[vti - 1];
               buffer->add(v, vn, vt);
            }
            else
               buffer->add(v, vn);
         }

         faceCount++;
            
         // Don't discard the next line
         continue;
      }

      // Discard the rest of the line
      getline(f, first);
   }

   Vector<float> dim = makeVector(xmax - xmin, ymax - ymin, zmax - zmin);
   
   log() << "Model loaded: " << vertices.size() << " vertices, "
         << faceCount << " faces";
   
   IModelPtr ptr(new Model(dim, buffer));

   theCache[cacheName] = ptr;
   return ptr;
}


