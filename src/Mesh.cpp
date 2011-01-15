//
//  Copyright (C) 2009-2011  Nick Gasson
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
   int frame_counter = 0;
   int triangle_count = 0;
}

// Concrete implementation of mesh buffers
struct MeshBuffer : IMeshBuffer {
   MeshBuffer();
   ~MeshBuffer() {}

   size_t vertex_count() const { return vertices.size(); }
   
   void add(const Vertex& vertex,
            const Normal& normal,
            const Colour& colour,
            const TexCoord& tex_coord);

   void add_quad(Vertex a, Vertex b, Vertex c, Vertex d,
                 Colour colour);
   void add_quad(Vertex a, Vertex b, Vertex c, Vertex d,
                 Normal na, Normal nb, Normal nc, Normal nd,
                 Colour colour);
      
   void bind(ITexturePtr texture);
   void merge(IMeshBufferPtr other, Vector<float> off, float y_angle);
   
   void print_stats() const;
   
   static MeshBuffer* get(IMeshBufferPtr a_ptr)
   {
      return polymorphic_cast<MeshBuffer*>(a_ptr.get());
   }

   static bool merge_vector(const Vector<float>& v1, const Vector<float>& v2)
   {
      return v1 == v2;
   }
   
   vector<Vertex> vertices;
   vector<Normal> normals;
   vector<Colour> colours;
   vector<Index> indices;
   vector<TexCoord> tex_coords;
   ITexturePtr texture;
   int reused;
};

MeshBuffer::MeshBuffer()
   : reused(0)
{
   
}

void MeshBuffer::bind(ITexturePtr tex)
{
   texture = tex;
}

void MeshBuffer::merge(IMeshBufferPtr other, Vector<float> off, float y_angle)
{
   const MeshBuffer& obuf = dynamic_cast<const MeshBuffer&>(*other);
   
   const size_t ibase = vertices.size();

   const Matrix<float, 4> translate =
      Matrix<float, 4>::translation(off.x, off.y, off.z);
   const Matrix<float, 4> rotate =
      Matrix<float, 4>::rotation(y_angle, 0.0f, 1.0f, 0.0f);

   const Matrix<float, 4> compose = translate * rotate;
   
   for (size_t i = 0; i < obuf.vertices.size(); i++) {
      const Vertex& v = obuf.vertices[i];
      const Normal& n = obuf.normals[i];
      
      vertices.push_back(compose.transform(v));
      normals.push_back(rotate.transform(n).normalise());

      if (obuf.texture) {
         colours.push_back(colour::WHITE);
      }
      else {
         const Colour& c = obuf.colours[i];
         colours.push_back(c);
      }
   }

   for (size_t i = 0; i < obuf.indices.size(); i++) {
      Index orig = obuf.indices[i];
      Index merged = orig + ibase;
      
      assert(orig < obuf.vertices.size());
      assert(merged < vertices.size());
      
      indices.push_back(merged);
   }
}

void MeshBuffer::print_stats() const
{
   debug() << "Mesh: " << vertices.size() << " vertices, "
	   << reused << " reused";
}

void MeshBuffer::add(const Vertex& vertex,
                     const Normal& normal,
                     const Colour& colour,
                     const TexCoord& a_tex_coord)
{
   // See if this vertex has already been added
   for (vector<Index>::iterator it = indices.begin();
	it != indices.end(); ++it) {
      if (merge_vector(vertex, vertices[*it])
          && merge_vector(normal, normals[*it])) {

	 const TexCoord& tc = tex_coords[*it];
         const bool same_tc = (approx_equal(tc.x, a_tex_coord.x)
                               && approx_equal(tc.y, a_tex_coord.y));

         const Colour& c = colours[*it];
	 const bool same_col = (approx_equal(c.r, colour.r)
                                && approx_equal(c.g, colour.g)
                                && approx_equal(c.b, colour.b));

         if (same_col && same_tc) {
            indices.push_back(*it);
	    reused++;
	    return;
         }
      }
   }
   
   const size_t index = vertices.size();
   vertices.push_back(vertex);
   normals.push_back(normal);
   tex_coords.push_back(a_tex_coord);
   colours.push_back(colour);
   indices.push_back(index);
}

void MeshBuffer::add_quad(Vertex a, Vertex b, Vertex c,
                          Vertex d, Colour colour)
{
   Vector<float> n1 = surface_normal(b, c, d);
   Vector<float> n2 = surface_normal(d, a, b);

   const TexCoord nulltc = make_point(0.0f, 0.0f);
   
   add(b, n1, colour, nulltc);
   add(c, n1, colour, nulltc);
   add(d, n1, colour, nulltc);

   add(d, n2, colour, nulltc);
   add(a, n2, colour, nulltc);
   add(b, n2, colour, nulltc);
}

void MeshBuffer::add_quad(Vertex a, Vertex b, Vertex c, Vertex d,
                          Normal na, Normal nb, Normal nc, Normal nd,
                          Colour colour)
{
   const TexCoord nulltc = make_point(0.0f, 0.0f);
      
   add(b, na, colour, nulltc);
   add(c, nb, colour, nulltc);
   add(d, nc, colour, nulltc);

   add(d, nd, colour, nulltc);
   add(a, na, colour, nulltc);
   add(b, nb, colour, nulltc);
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

// Get the vertex data out of a mesh buffer into a VertexData array
static void copy_vertex_data(const MeshBuffer* buf, VertexData* vertex_data)
{
   for (size_t i = 0; i < buf->vertices.size(); i++) {
      VertexData* vd = &vertex_data[i];
      
      vd->x = buf->vertices[i].x;
      vd->y = buf->vertices[i].y;
      vd->z = buf->vertices[i].z;
         
      vd->nx = buf->normals[i].x;
      vd->ny = buf->normals[i].y;
      vd->nz = buf->normals[i].z;
      
      if (buf->texture) {
         vd->tx = buf->tex_coords[i].x;
         vd->ty = 1.0f - buf->tex_coords[i].y;
      }
      
      vd->r = buf->colours[i].r;
      vd->g = buf->colours[i].g;
      vd->b = buf->colours[i].b;
   }
}

// Implementation of meshes using client side vertex arrays
class VertexArrayMesh : public IMesh {
public:
   VertexArrayMesh(IMeshBufferPtr a_buffer);
   ~VertexArrayMesh();

   void render() const;

private:
   ITexturePtr texture;
   size_t my_vertex_count;
   VertexData* my_vertex_data;
   size_t my_index_count;
   GLushort* my_indices;
};

VertexArrayMesh::VertexArrayMesh(IMeshBufferPtr a_buffer)
{
   const MeshBuffer* buf = MeshBuffer::get(a_buffer);

   texture = buf->texture;
 
   my_vertex_count = buf->vertices.size();
   my_vertex_data = new VertexData[my_vertex_count];

   copy_vertex_data(buf, my_vertex_data);
   
   my_index_count = buf->indices.size();
   my_indices = new GLushort[my_index_count];

   copy(buf->indices.begin(), buf->indices.end(), my_indices);
}

VertexArrayMesh::~VertexArrayMesh()
{
   delete[] my_vertex_data;
   delete[] my_indices;
}

void VertexArrayMesh::render() const
{
   glPushAttrib(GL_ENABLE_BIT);
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   if (!glIsEnabled(GL_CULL_FACE))
      glEnable(GL_CULL_FACE);

   if (glIsEnabled(GL_BLEND))
       glDisable(GL_BLEND);
   
   if (texture) {
      glEnable(GL_TEXTURE_2D);
      texture->bind();
      
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData),
                        reinterpret_cast<GLvoid*>(&my_vertex_data->tx));
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }       

   glEnable(GL_COLOR_MATERIAL);
       
   glEnableClientState(GL_COLOR_ARRAY);
   glColorPointer(3, GL_FLOAT, sizeof(VertexData),
                  reinterpret_cast<GLvoid*>(&my_vertex_data->r));
   
   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(my_vertex_data));
   glNormalPointer(GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(&my_vertex_data->nx));

   glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(my_index_count),
      GL_UNSIGNED_SHORT, my_indices);
   
   glPopClientAttrib();
   glPopAttrib();
}

// Implementation of meshes using server side VBOs
class VBOMesh : public IMesh {
public:
   VBOMesh(IMeshBufferPtr a_buffer);
   ~VBOMesh();

   void render() const;
private:
   GLuint vbo_buf, index_buf;
   ITexturePtr texture;
   size_t index_count;
};

VBOMesh::VBOMesh(IMeshBufferPtr a_buffer)
{
   // Get the data out of the buffer;
   const MeshBuffer* buf = MeshBuffer::get(a_buffer);

   texture = buf->texture;
 
   const size_t vertex_count = buf->vertices.size();
   VertexData* p_vertex_data = new VertexData[vertex_count];

   copy_vertex_data(buf, p_vertex_data);

   // Generate the VBO   
   glGenBuffersARB(1, &vbo_buf);
   glBindBufferARB(GL_ARRAY_BUFFER, vbo_buf);
   glBufferDataARB(GL_ARRAY_BUFFER, vertex_count * sizeof(VertexData),
      NULL, GL_STATIC_DRAW);

   // Copy the vertex data in
   glBufferSubDataARB(GL_ARRAY_BUFFER, 0,
      vertex_count * sizeof(VertexData), p_vertex_data);

   // Copy the indices into a temporary array
   index_count = buf->indices.size();
   GLshort* p_indices = new GLshort[index_count];

   copy(buf->indices.begin(), buf->indices.end(), p_indices);
   
   // Build the index buffer
   glGenBuffersARB(1, &index_buf);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, index_buf);
   glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER, index_count * sizeof(GLushort),
      NULL, GL_STATIC_DRAW);
   glBufferSubDataARB(GL_ELEMENT_ARRAY_BUFFER, 0,
      index_count * sizeof(GLushort), p_indices);

   glBindBufferARB(GL_ARRAY_BUFFER, 0);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, 0);
   
   delete[] p_vertex_data;
   delete[] p_indices;
}

VBOMesh::~VBOMesh()
{
   glDeleteBuffersARB(1, &vbo_buf);
   glDeleteBuffersARB(1, &index_buf);
}

void VBOMesh::render() const
{
   glPushAttrib(GL_ENABLE_BIT);
   glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

   if (!glIsEnabled(GL_CULL_FACE))
      glEnable(GL_CULL_FACE);

   glBindBufferARB(GL_ARRAY_BUFFER, vbo_buf);
   glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER, index_buf);

   if (glIsEnabled(GL_BLEND))
      glDisable(GL_BLEND);

   if (texture) {
      glEnable(GL_TEXTURE_2D);
      texture->bind();
      
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);
      glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData),
                        reinterpret_cast<GLvoid*>(offsetof(VertexData, tx)));
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }
      
   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
   
   glEnableClientState(GL_COLOR_ARRAY);
   glColorPointer(3, GL_FLOAT, sizeof(VertexData),
                  reinterpret_cast<GLvoid*>(offsetof(VertexData, r)));

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);

   // Pointers are relative to start of VBO
   glNormalPointer(GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(offsetof(VertexData, nx)));
   glVertexPointer(3, GL_FLOAT, sizeof(VertexData),
      reinterpret_cast<GLvoid*>(0));
   
   glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(index_count),
                  GL_UNSIGNED_SHORT, 0);

   glPopClientAttrib();
   glPopAttrib();

   ::triangle_count += index_count / 3;
}

IMeshPtr make_mesh(IMeshBufferPtr buffer)
{
   buffer->print_stats();
   
   // Prefer VBOs for all meshes
   if (GLEW_ARB_vertex_buffer_object)
      return IMeshPtr(new VBOMesh(buffer));
   else
      return IMeshPtr(new VertexArrayMesh(buffer));
}

IMeshBufferPtr make_mesh_buffer()
{
   return IMeshBufferPtr(new MeshBuffer);
}

void update_render_stats()
{
   ::frame_counter++;
}

int get_average_triangle_count()
{
   if (::frame_counter == 0)
      return 0;
   else {
      int avg = ::triangle_count / ::frame_counter;
      ::triangle_count = ::frame_counter = 0;
      return avg;
   }
}
