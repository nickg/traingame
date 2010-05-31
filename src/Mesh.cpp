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

#include <GL/glew.h>

#include "IMesh.hpp"
#include "ITexture.hpp"
#include "ILogger.hpp"
#include "OpenGLHelper.hpp"
#include "Matrix.hpp"

#include <vector>
#include <stdexcept>

#include <boost/cast.hpp>
#include <boost/static_assert.hpp>

using namespace boost;

namespace {
   int frameCounter = 0;
   int triangleCount = 0;
}

// Concrete implementation of mesh buffers
struct MeshBuffer : IMeshBuffer {
   MeshBuffer();
   ~MeshBuffer() {}

   size_t vertexCount() const { return vertices.size(); }
   
   void add(const Vertex& vertex, const Normal& normal);
   void add(const Vertex& vertex, const Normal& normal,
      const TexCoord& aTexCoord);
   void add(const Vertex& vertex, const Normal& normal,
      const Colour& colour);

   void addQuad(Vertex a, Vertex b, Vertex c, Vertex d,
      Colour colour);
   void addQuad(Vertex a, Vertex b, Vertex c, Vertex d,
      Normal na, Normal nb, Normal nc, Normal nd,
      Colour colour);
      
   void bindMaterial(const Material& aMaterial);
   void merge(IMeshBufferPtr other, Vector<float> off, float yAngle);
   
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

void MeshBuffer::merge(IMeshBufferPtr other, Vector<float> off, float yAngle)
{
   const MeshBuffer& obuf = dynamic_cast<const MeshBuffer&>(*other);
   
   const size_t ibase = vertices.size();

   const Matrix<float, 4> translate =
      Matrix<float, 4>::translation(off.x, off.y, off.z);
   const Matrix<float, 4> rotate =
      Matrix<float, 4>::rotation(yAngle, 0.0f, 1.0f, 0.0f);

   const Matrix<float, 4> compose = translate * rotate;
   
   for (size_t i = 0; i < obuf.vertices.size(); i++) {
      const Vertex& v = obuf.vertices[i];
      const Normal& n = obuf.normals[i];
      
      vertices.push_back(compose.transform(v));
      normals.push_back(rotate.transform(n).normalise());

      if (obuf.hasTexture) {
         colours.push_back(colour::WHITE);
      }
      else {
         const Colour& c = obuf.colours[i];
         colours.push_back(c);
      }
   }

   for (size_t i = 0; i < obuf.indices.size(); i++) {
      Index orig = obuf.indices[i];
      indices.push_back(orig + ibase);
   }
}

void MeshBuffer::printStats() const
{
   debug() << "Mesh: " << vertices.size() << " vertices, "
	   << reused << " reused";
}

void MeshBuffer::add(const Vertex& vertex, const Normal& normal)
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
      if (mergeVector(vertex, vertices[*it])
	 && mergeVector(normal, normals[*it])) {
         
	 const Colour& other = colours[*it];
	 if (abs(other.r - material.diffuseR) < 0.01f
	    && abs(other.g - material.diffuseG) < 0.01f
	    && abs(other.b - material.diffuseB) < 0.01f) {
         
	    indices.push_back(*it);
	    reused++;
	    return;
	 }
      }
   }
   
   const size_t index = vertices.size();
   vertices.push_back(vertex);
   normals.push_back(normal);
   indices.push_back(index);
   colours.push_back(
      makeColour(material.diffuseR,
         material.diffuseG,
         material.diffuseB));
}         

void MeshBuffer::add(const Vertex& vertex, const Normal& normal,
   const Colour& colour)
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
      if (mergeVector(vertex, vertices[*it])
	 && mergeVector(normal, normals[*it])) {

	 const Colour& other = colours[*it];
	 if (abs(other.r - colour.r) < 0.01f
	    && abs(other.g - colour.g) < 0.01f
	    && abs(other.b - colour.b) < 0.01f) {
         
	    indices.push_back(*it);
	    reused++;
	    return;
	 }
      }
   }
   
   const size_t index = vertices.size();
   vertices.push_back(vertex);
   normals.push_back(normal);
   colours.push_back(colour);
   indices.push_back(index);
}

void MeshBuffer::add(const Vertex& vertex, const Normal& normal,
   const TexCoord& aTexCoord)
{
   if (!hasTexture)
      throw runtime_error(
	 "MeshBuffer::add called with a texture coordinate "
	 "on a mesh without a texture");
   
   // See if this vertex has already been added
   for (vector<Index>::iterator it = indices.begin();
	it != indices.end(); ++it) {
      if (mergeVector(vertex, vertices[*it])
	 && mergeVector(normal, normals[*it])) {
	 TexCoord& tc = texCoords[*it];
	 if (abs(tc.x - aTexCoord.x) < 0.001f
	    && abs(tc.y - aTexCoord.y) < 0.001f) {
	    indices.push_back(*it);
	    reused++;
	    return;
	 }
      }
   }
   
   const size_t index = vertices.size();
   vertices.push_back(vertex);
   normals.push_back(normal);
   texCoords.push_back(aTexCoord);
   indices.push_back(index);
}

void MeshBuffer::addQuad(Vertex a, Vertex b, Vertex c,
   Vertex d, Colour colour)
{
   Vector<float> n1 = surfaceNormal(b, c, d);
   Vector<float> n2 = surfaceNormal(d, a, b);

   add(b, n1, colour);
   add(c, n1, colour);
   add(d, n1, colour);

   add(d, n2, colour);
   add(a, n2, colour);
   add(b, n2, colour);
}

void MeshBuffer::addQuad(Vertex a, Vertex b, Vertex c, Vertex d,
   Normal na, Normal nb, Normal nc, Normal nd,
   Colour colour)
{
   
   add(b, na, colour);
   add(c, nb, colour);
   add(d, nc, colour);

   add(d, nd, colour);
   add(a, na, colour);
   add(b, nb, colour);
}

// Default material
Material::Material()
   : diffuseR(1.0f), diffuseG(1.0f), diffuseB(1.0f),
     ambientR(1.0f), ambientG(1.0f), ambientB(1.0f),
     specularR(0.0f), specularG(0.0f), specularB(0.0f)
{
}

void Material::apply() const
{
   if (texture) {
      glEnable(GL_TEXTURE_2D);
      texture->bind();
      
      glEnable(GL_COLOR_MATERIAL);
      glColor3f(1.0f, 1.0f, 1.0f);
   }
   else {
      glDisable(GL_COLOR_MATERIAL);
      glDisable(GL_TEXTURE_2D);
   
      float diffuse[] = { diffuseR, diffuseG, diffuseB, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, diffuse);
      
      float ambient[] = { ambientR, ambientG, ambientB, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, ambient);
      
      // Note we're ignoring the specular values in the model
      float specular[] = { 0, 0, 0, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
      
      float emission[] = { 0, 0, 0, 1 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, emission);
   }
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

   if (buf->hasMaterial)
      buf->material.apply();
   else
      glEnable(GL_COLOR_MATERIAL);
   
   glBegin(GL_TRIANGLES);

   vector<MeshBuffer::Index>::const_iterator it;
   for (it = buf->indices.begin();
	it != buf->indices.end(); ++it) {

      if (!buf->hasMaterial) {
	 gl::colour(buf->colours[*it]);
      }
      
      const MeshBuffer::Normal& n = buf->normals[*it];
      glNormal3f(n.x, n.y, n.z);
      
      const MeshBuffer::Vertex& v = buf->vertices[*it];
      glVertex3f(v.x, v.y, v.z);
   }        
           
   glEnd();
   
   for (it = buf->indices.begin();
	it != buf->indices.end(); ++it) {
      const MeshBuffer::Vertex& v = buf->vertices[*it];
      const MeshBuffer::Normal& n = buf->normals[*it];
      drawNormal(v, n);
   }     

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

// Packed vertex data used by vertex array and VBO mesh implementations
struct VertexData {
   float x, y, z;
   float nx, ny, nz;
   float tx, ty;
   float r, g, b;
   float padding[5];   // Best performance on some cards if 32-byte aligned
};

BOOST_STATIC_ASSERT(sizeof(VertexData) == 64);

namespace {
   // Get the vertex data out of a mesh buffer into a VertexData array
   void copyVertexData(const MeshBuffer* buf, VertexData* vertexData)
   {
      for (size_t i = 0; i < buf->vertices.size(); i++) {
	 VertexData* vd = &vertexData[i];
         
	 vd->x = buf->vertices[i].x;
	 vd->y = buf->vertices[i].y;
	 vd->z = buf->vertices[i].z;
         
	 vd->nx = buf->normals[i].x;
	 vd->ny = buf->normals[i].y;
	 vd->nz = buf->normals[i].z;
         
	 if (buf->hasTexture) {
	    vd->tx = buf->texCoords[i].x;
	    vd->ty = 1.0f - buf->texCoords[i].y;
	 }
         else {
	    vd->r = buf->colours[i].r;
	    vd->g = buf->colours[i].g;
	    vd->b = buf->colours[i].b;
	 }
      }
   }
}

// Implementation of meshes using client side vertex arrays
class VertexArrayMesh : public IMesh {
public:
   VertexArrayMesh(IMeshBufferPtr aBuffer);
   ~VertexArrayMesh();

   void render() const;
private:

   Material material;
   bool hasMaterial, hasTexture;
   size_t myVertexCount;
   VertexData* myVertexData;
   size_t myIndexCount;
   GLushort* myIndices;
};

VertexArrayMesh::VertexArrayMesh(IMeshBufferPtr aBuffer)
{
   const MeshBuffer* buf = MeshBuffer::get(aBuffer);

   material = buf->material;
   hasMaterial = buf->hasMaterial;
   hasTexture = buf->hasTexture;
 
   myVertexCount = buf->vertices.size();
   myVertexData = new VertexData[myVertexCount];

   copyVertexData(buf, myVertexData);
   
   myIndexCount = buf->indices.size();
   myIndices = new GLushort[myIndexCount];

   copy(buf->indices.begin(), buf->indices.end(), myIndices);
}

VertexArrayMesh::~VertexArrayMesh()
{
   delete[] myVertexData;
   delete[] myIndices;
}

void VertexArrayMesh::render() const
{
   glPushAttrib(GL_ENABLE_BIT);
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   glDisable(GL_BLEND);
   
   if (hasMaterial)
      material.apply();
   else {
      glEnable(GL_COLOR_MATERIAL);

      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(3, GL_FLOAT, sizeof(VertexData),
	 reinterpret_cast<GLvoid*>(&myVertexData->r));
   }
   
   if (hasTexture) {
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData),
	 reinterpret_cast<GLvoid*>(&myVertexData->tx));
   }
      
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(myVertexData));
   glNormalPointer(GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(&myVertexData->nx));

   glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(myIndexCount),
      GL_UNSIGNED_SHORT, myIndices);
   
   glPopClientAttrib();
   glPopAttrib();
}

// Implementation of meshes using server side VBOs
class VBOMesh : public IMesh {
public:
   VBOMesh(IMeshBufferPtr aBuffer);
   ~VBOMesh();

   void render() const;
private:
   GLuint vboBuf, indexBuf;
   ITexturePtr texture;
   size_t indexCount;
};

VBOMesh::VBOMesh(IMeshBufferPtr aBuffer)
{
   // Get the data out of the buffer;
   const MeshBuffer* buf = MeshBuffer::get(aBuffer);

   texture = buf->material.texture;
 
   const size_t vertexCount = buf->vertices.size();
   VertexData* pVertexData = new VertexData[vertexCount];

   copyVertexData(buf, pVertexData);

   // Generate the VBO   
   glGenBuffersARB(1, &vboBuf);
   glBindBufferARB(GL_ARRAY_BUFFER, vboBuf);
   glBufferDataARB(GL_ARRAY_BUFFER, vertexCount * sizeof(VertexData),
      NULL, GL_STATIC_DRAW);

   // Copy the vertex data in
   glBufferSubDataARB(GL_ARRAY_BUFFER, 0,
      vertexCount * sizeof(VertexData), pVertexData);

   // Copy the indices into a temporary array
   indexCount = buf->indices.size();
   GLshort* pIndices = new GLshort[indexCount];

   copy(buf->indices.begin(), buf->indices.end(), pIndices);
   
   // Build the index buffer
   glGenBuffersARB(1, &indexBuf);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, indexBuf);
   glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(GLushort),
      NULL, GL_STATIC_DRAW);
   glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER, 0,
      indexCount * sizeof(GLushort), pIndices);

   glBindBufferARB(GL_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   
   delete[] pVertexData;
   delete[] pIndices;
}

VBOMesh::~VBOMesh()
{
   glDeleteBuffersARB(1, &vboBuf);
   glDeleteBuffersARB(1, &indexBuf);
}

void VBOMesh::render() const
{
   glPushAttrib(GL_ENABLE_BIT);
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   if (!glIsEnabled(GL_CULL_FACE))
      glEnable(GL_CULL_FACE);

   glBindBufferARB(GL_ARRAY_BUFFER, vboBuf);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, indexBuf);

   if (glIsEnabled(GL_BLEND))
      glDisable(GL_BLEND);

   if (texture) {
      glEnable(GL_TEXTURE_2D);
      texture->bind();
      
      glEnable(GL_COLOR_MATERIAL);
      glColor3f(1.0f, 1.0f, 1.0f);
   }
   else {
      glDisable(GL_TEXTURE_2D);
      
      glEnable(GL_COLOR_MATERIAL);
      glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

      glEnableClientState(GL_COLOR_ARRAY);
      glColorPointer(3, GL_FLOAT, sizeof(VertexData),
	 reinterpret_cast<GLvoid*>(offsetof(VertexData, r)));
   }

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);

   // Pointers are relative to start of VBO
   glNormalPointer(GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(offsetof(VertexData, nx)));
   glVertexPointer(3, GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(0));
   
   glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
      GL_UNSIGNED_SHORT, 0);

   glPopClientAttrib();
   glPopAttrib();

   ::triangleCount += indexCount / 3;
}

IMeshPtr makeMesh(IMeshBufferPtr aBuffer)
{
   //aBuffer->printStats();
   
   // Prefer VBOs for all meshes
   if (GLEW_ARB_vertex_buffer_object)
      return IMeshPtr(new VBOMesh(aBuffer));
   else
      return IMeshPtr(new VertexArrayMesh(aBuffer));
}

IMeshBufferPtr makeMeshBuffer()
{
   return IMeshBufferPtr(new MeshBuffer);
}

void updateRenderStats()
{
   ::frameCounter++;
}

int getAverageTriangleCount()
{
   int avg = ::triangleCount / ::frameCounter;
   ::triangleCount = ::frameCounter = 0;
   return avg;
}
