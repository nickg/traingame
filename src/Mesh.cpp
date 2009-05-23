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

#include "IMesh.hpp"
#include "ITexture.hpp"
#include "ILogger.hpp"

#include <vector>
#include <stdexcept>

#include <GL/gl.h>
#include <boost/cast.hpp>

using namespace std;
using namespace boost;

// Concrete implementation of mesh buffers
struct MeshBuffer : IMeshBuffer {
   MeshBuffer();
   ~MeshBuffer() {}

   void add(const Vertex& aVertex, const Normal& aNormal);
   void add(const Vertex& aVertex, const Normal& aNormal,
            const TexCoord& aTexCoord);
   void add(const Vertex& aVertex, const Normal& aNormal,
            const Colour& aColour);

   void addQuad(Vertex a, Vertex b, Vertex c, Vertex d,
                Colour aColour);
      
   void bindMaterial(const Material& aMaterial);
   
   void printStats() const;
   
   static MeshBuffer* get(IMeshBufferPtr aPtr)
   {
      return polymorphic_cast<MeshBuffer*>(aPtr.get());
   }

   static bool mergeVector(const Vector<float>& v1, const Vector<float>& v2)
   {
      const float tolerance = 0.0001f;
      
      return abs(v1.x - v2.x) < tolerance
         && abs(v1.y - v2.y) < tolerance
         && abs(v1.z - v2.z) < tolerance;
   }
   
   vector<Vertex> vertices;
   vector<Normal> normals;
   vector<Colour> colours;
   vector<Index> indices;
   vector<TexCoord> texCoords;
   bool hasTexture, hasMaterial;
   Material material;
   int reused;
};

MeshBuffer::MeshBuffer()
   : hasTexture(false), hasMaterial(false), reused(0)
{
   
}

void MeshBuffer::bindMaterial(const Material& aMaterial)
{
   material = aMaterial;
   hasTexture = aMaterial.texture;
   hasMaterial = true;
}

void MeshBuffer::printStats() const
{
   debug() << "Mesh: " << vertices.size() << " vertices, "
           << reused << " reused";
}

void MeshBuffer::add(const Vertex& aVertex, const Normal& aNormal)
{
   if (hasTexture)
      throw runtime_error("MeshBuffer::add called without texture coordinate "
                          "on a mesh which has a texture");

   if (!hasMaterial)
      throw runtime_error("MeshBuffer::add called without colour on a mesh "
                          " without a material");
   
   // See if this vertex has already been added
   for (vector<Index>::iterator it = indices.begin();
        it != indices.end(); ++it) {
      if (mergeVector(aVertex, vertices[*it])
          && mergeVector(aNormal, normals[*it])) {
         indices.push_back(*it);
         reused++;
         return;
      }
   }
   
   const int index = vertices.size();
   vertices.push_back(aVertex);
   normals.push_back(aNormal);
   indices.push_back(index);
}

void MeshBuffer::add(const Vertex& aVertex, const Normal& aNormal,
                     const Colour& aColour)
{
   
   if (hasTexture)
      throw runtime_error("MeshBuffer::add called without texture coordinate "
                          "on a mesh which has a texture");

   if (hasMaterial)
      throw runtime_error("MeshBuffer::add called with a colour on a mesh "
                          " with a material");
   
   // See if this vertex has already been added
   for (vector<Index>::iterator it = indices.begin();
        it != indices.end(); ++it) {
      if (mergeVector(aVertex, vertices[*it])
          && mergeVector(aNormal, normals[*it])) {

         const Colour& other = colours[*it];
         if (abs(std::get<0>(other) - std::get<0>(aColour)) < 0.01f
             && abs(std::get<1>(other) - std::get<1>(aColour)) < 0.01f
             && abs(std::get<2>(other) - std::get<2>(aColour)) < 0.01f) {
         
            indices.push_back(*it);
            reused++;
            return;
         }
      }
   }
   
   const int index = vertices.size();
   vertices.push_back(aVertex);
   normals.push_back(aNormal);
   colours.push_back(aColour);
   indices.push_back(index);
}

void MeshBuffer::add(const Vertex& aVertex, const Normal& aNormal,
                     const TexCoord& aTexCoord)
{
   if (!hasTexture)
      throw runtime_error("MeshBuffer::add called with a texture coordinate "
                          "on a mesh without a texture");
   
   // See if this vertex has already been added
   for (vector<Index>::iterator it = indices.begin();
        it != indices.end(); ++it) {
      if (mergeVector(aVertex, vertices[*it])
          && mergeVector(aNormal, normals[*it])) {
         TexCoord& tc = texCoords[*it];
         if (abs(tc.x - aTexCoord.x) < 0.001f
             && abs(tc.y - aTexCoord.y) < 0.001f) {
            indices.push_back(*it);
            reused++;
            return;
         }
      }
   }
   
   const int index = vertices.size();
   vertices.push_back(aVertex);
   normals.push_back(aNormal);
   texCoords.push_back(aTexCoord);
   indices.push_back(index);
}

void MeshBuffer::addQuad(Vertex a, Vertex b, Vertex c, Vertex d, Colour aColour)
{
   Vector<float> n1 = surfaceNormal(b, c, d);
   Vector<float> n2 = surfaceNormal(d, a, b);

   add(b, n1, aColour);
   add(c, n1, aColour);
   add(d, n1, aColour);

   add(d, n2, aColour);
   add(a, n2, aColour);
   add(b, n2, aColour);
}

// Default material
Material::Material()
  : diffuseR(1.0f), diffuseG(1.0f), diffuseB(1.0f),
    ambientR(1.0f), ambientG(1.0f), ambientB(1.0f),
    specularR(0.0f), specularG(0.0f), specularB(0.0f)
{
}

// Simple implementation using display lists
class DisplayListMesh : public IMesh {
public:
   DisplayListMesh(IMeshBufferPtr aBuffer);
   ~DisplayListMesh();

   void render() const;
private:
   GLuint myDisplayList;
};

DisplayListMesh::DisplayListMesh(IMeshBufferPtr aBuffer)
{
   myDisplayList = glGenLists(1);

   glNewList(myDisplayList, GL_COMPILE);

   const MeshBuffer* buf = MeshBuffer::get(aBuffer);
   const Material& m = buf->material;

   if (buf->hasMaterial) {
      glDisable(GL_COLOR_MATERIAL);
      
      if (buf->hasTexture) {
         glEnable(GL_TEXTURE_2D);
         m.texture->bind();
      }
      else
         glDisable(GL_TEXTURE_2D);
      
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
   else
      glEnable(GL_COLOR_MATERIAL);
   
   glBegin(GL_TRIANGLES);

   vector<MeshBuffer::Index>::const_iterator it;
   for (it = buf->indices.begin();
        it != buf->indices.end(); ++it) {

      if (!buf->hasMaterial) {
         const MeshBuffer::Colour& c = buf->colours[*it];
         glColor3f(get<0>(c), get<1>(c), get<2>(c));
      }
      
      const MeshBuffer::Normal& n = buf->normals[*it];
      glNormal3f(n.x, n.y, n.z);
      
      const MeshBuffer::Vertex& v = buf->vertices[*it];
      glVertex3f(v.x, v.y, v.z);
   }        
           
   glEnd();
   
   /*for (it = buf->indices.begin();
        it != buf->indices.end(); ++it) {
      const MeshBuffer::Vertex& v = buf->vertices[*it];
      const MeshBuffer::Normal& n = buf->normals[*it];
      drawNormal(v, n);
      } */       

   glEndList();
}

DisplayListMesh::~DisplayListMesh()
{
   glDeleteLists(myDisplayList, 1);
}

void DisplayListMesh::render() const
{   
   glPushAttrib(GL_ENABLE_BIT);

   glDisable(GL_BLEND);
   glEnable(GL_CULL_FACE);
   
   glCallList(myDisplayList);

   glPopAttrib();
}

IMeshPtr makeMesh(IMeshBufferPtr aBuffer)
{
   aBuffer->printStats();
   return IMeshPtr(new DisplayListMesh(aBuffer));
}

IMeshBufferPtr makeMeshBuffer()
{
   return IMeshBufferPtr(new MeshBuffer);
}
