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

using namespace boost;

// Cache of already loaded models
namespace {
   typedef map<string, IModelPtr> ModelCache;
   ModelCache the_cache;
}

// Abstracts a WaveFront material file
class MaterialFile {
public:
   MaterialFile(const string& a_file_name, IResourcePtr a_res);
   ~MaterialFile() {}

   const Material& get(const string& a_name) const;
private:
   typedef map<string, Material> MaterialSet;
   MaterialSet my_materials;
};

typedef std::tr1::shared_ptr<MaterialFile> MaterialFilePtr;

MaterialFile::MaterialFile(const string& a_file_name, IResourcePtr a_res)
{
   IResource::Handle h = a_res->open_file(a_file_name);
   
   log() << "Loading materials from " << h.file_name();

   istream& is = h.rstream();
   
   string active_material;
   while (!is.eof()) {
      string word;
      is >> word;

      if (word[0] == '#') {
         // Comment
         continue;
      }
      else if (word == "newmtl") {
         is >> active_material;

         my_materials[active_material] = Material();
      }
      else if (word == "map_Kd") {
         // Texture
         is >> word;
         my_materials[active_material].texture = load_texture(a_res, word);
      }
      else if (word == "Kd") {
         // Diffuse colour
         Material& m = my_materials[active_material];
         is >> m.diffuseR >> m.diffuseG >> m.diffuseB;
      }
      else if (word == "Ka") {
         // Ambient colour
         Material& m = my_materials[active_material];
         is >> m.ambientR >> m.ambientG >> m.ambientB;
      }
      else if (word == "Ks") {
         // Specular colour
         Material& m = my_materials[active_material];
         is >> m.specularR >> m.specularG >> m.specularB;
      }
      else {
         // Ignore it
         continue;
      }
   }
}

const Material& MaterialFile::get(const string& a_name) const
{
   MaterialSet::const_iterator it = my_materials.find(a_name);
   if (it == my_materials.end())
      throw runtime_error("No material named " + a_name);

   return (*it).second;
}

class Model : public IModel {
public:
   Model(const Vector<float>& dim, const IMeshBufferPtr buf)
      : dimensions_(dim), buffer(buf)
   {}
   ~Model();

   // IModel interface
   void render() const;
   void cache();
   void merge(IMeshBufferPtr into, Vector<float> off, float y_angle) const;
   Vector<float> dimensions() const { return dimensions_; }
   
private:
   void compile_mesh() const;
   
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
      compile_mesh();
}

void Model::render() const
{
   if (!mesh)
      compile_mesh();
   
   mesh->render();
}

void Model::merge(IMeshBufferPtr into, Vector<float> off, float y_angle) const
{
   into->merge(buffer, off, y_angle);
}   

void Model::compile_mesh() const
{
   // Const as may be called during render
   mesh = make_mesh(buffer);
}

// Load a model from a resource
IModelPtr load_model(IResourcePtr a_res,
                     const string& a_file_name,
                     float a_scale,
                     Vector<float> shift)
{
   // Make a unique cache name
   const string cache_name = a_res->name() + ":" + a_file_name;

   // Check the cache for the model
   ModelCache::iterator it = the_cache.find(cache_name);
   if (it != the_cache.end())
      return (*it).second;

   // Not in the cache, load it from the resource
   IResource::Handle h = a_res->open_file(a_file_name);
   log() << "Loading model " << h.file_name();

   vector<IMeshBuffer::Vertex> vertices;
   vector<IMeshBuffer::Normal> normals;
   vector<IMeshBuffer::TexCoord> texture_offs;

   IMeshBufferPtr buffer = make_mesh_buffer();

   bool found_vertex = false;
   float ymin = 0, ymax = 0, xmin = 0, xmax = 0,
      zmin = 0, zmax = 0;
   int face_count = 0;

   MaterialFilePtr material_file;

   ifstream& f = h.rstream();
   
   while (!f.eof()) {
      string first;
      f >> first;

      if (first[0] == '#') {
         // Comment
      }
      else if (first == "o") {
         // New object
         string obj_name;
         f >> obj_name;
      }
      else if (first == "mtllib") {
         // Material file
         string file_name;
         f >> file_name;
         
         material_file =
            MaterialFilePtr(new MaterialFile(file_name, a_res));
      }
      else if (first == "v") {
         // Vertex
         float x, y, z;
         f >> x >> y >> z;

         x += shift.x;
         y += shift.y;
         z += shift.z;

         x *= a_scale;
         y *= a_scale;
         z *= a_scale;

         if (found_vertex) {
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

            found_vertex = true;
         }

         vertices.push_back(make_vector(x, y, z));
      }
      else if (first == "vn") {
         // Normal
         float x, y, z;
         f >> x >> y >> z;
         
         normals.push_back(make_vector(x, y, z));
      }
      else if (first == "vt") {
         // Texture coordinate
         float x, y;
         f >> x >> y;

         texture_offs.push_back(make_point(x, y));
      }
      else if (first == "g") {
         // Groups used to correspond to sub-meshes but now
         // the whole model is compiled into a single mesh
      }
      else if (first == "usemtl") {
         // Set the material for this group
         string material_name;
         f >> material_name;
         
         if (material_file) {
            assert(buffer);
            buffer->bind_material(material_file->get(material_name));
         }
      }
      else if (first == "f") {
         // Face
         string line;
         getline(f, line);
         istringstream ss(line);

         int v_in_thisFace = 0;
         
         while (!ss.eof()) {
            char delim1, delim2;
            unsigned vi, vti, vni;
            ss >> vi >> delim1;
            if (ss.fail())
               break;

            if (++v_in_thisFace > 3)
               warn () << "All model faces must be triangles "
                       << "(face with " << v_in_thisFace << " vertices)";

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
            
            if (vti - 1 < texture_offs.size()) {
               Point<float>& vt = texture_offs[vti - 1];
               buffer->add(v, vn, vt);
            }
            else
               buffer->add(v, vn);
         }

         face_count++;
            
         // Don't discard the next line
         continue;
      }

      // Discard the rest of the line
      getline(f, first);
   }

   Vector<float> dim = make_vector(xmax - xmin, ymax - ymin, zmax - zmin);
   
   log() << "Model loaded: " << vertices.size() << " vertices, "
         << face_count << " faces";
   
   IModelPtr ptr(new Model(dim, buffer));

   the_cache[cache_name] = ptr;
   return ptr;
}


