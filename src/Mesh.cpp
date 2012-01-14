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

   // A chunk is a subset of the mesh bound to a particular texture
   struct Chunk {
      vector<Vertex> vertices;
      vector<Normal> normals;
      vector<Colour> colours;
      vector<Index> indices;
      vector<TexCoord> tex_coords;
      ITexturePtr texture;
   };
   typedef shared_ptr<Chunk> ChunkPtr;

   size_t vertex_count() const;
   size_t index_count() const;

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

   ChunkPtr find_chunk(ITexturePtr tex) const;

   static MeshBuffer* get(IMeshBufferPtr a_ptr)
   {
      return polymorphic_cast<MeshBuffer*>(a_ptr.get());
   }

   vector<ChunkPtr> chunks;
   ChunkPtr active_chunk;

   int reused;
};

MeshBuffer::MeshBuffer()
   : reused(0)
{

}

void MeshBuffer::bind(ITexturePtr tex)
{
   for (vector<ChunkPtr>::iterator it = chunks.begin();
        it != chunks.end(); ++it) {
      if ((*it)->texture == tex) {
         active_chunk = *it;
         return;
      }
   }

   // Must create new chunk for this texture
   active_chunk = ChunkPtr(new Chunk);
   active_chunk->texture = tex;

   chunks.push_back(active_chunk);
}

size_t MeshBuffer::vertex_count() const
{
   size_t sum = 0;

   for (vector<ChunkPtr>::const_iterator it = chunks.begin();
        it != chunks.end(); ++it)
      sum += (*it)->vertices.size();

   return sum;
}

size_t MeshBuffer::index_count() const
{
   size_t sum = 0;

   for (vector<ChunkPtr>::const_iterator it = chunks.begin();
        it != chunks.end(); ++it)
      sum += (*it)->indices.size();

   return sum;
}

MeshBuffer::ChunkPtr MeshBuffer::find_chunk(ITexturePtr tex) const
{
   for (vector<ChunkPtr>::const_iterator it = chunks.begin();
          it != chunks.end(); ++it) {
      if ((*it)->texture == tex)
         return *it;
   }

   return ChunkPtr();
}

void MeshBuffer::merge(IMeshBufferPtr other, Vector<float> off, float y_angle)
{
   const MeshBuffer& obuf = dynamic_cast<const MeshBuffer&>(*other);

   for (vector<ChunkPtr>::const_iterator it = obuf.chunks.begin();
        it != obuf.chunks.end(); ++it) {

      ChunkPtr target_chunk = find_chunk((*it)->texture);
      if (!target_chunk) {
         target_chunk = ChunkPtr(new Chunk);
         target_chunk->texture = (*it)->texture;

         chunks.push_back(target_chunk);
      }

      const size_t ibase = target_chunk->vertices.size();

      const Matrix<float, 4> translate =
         Matrix<float, 4>::translation(off.x, off.y, off.z);
      const Matrix<float, 4> rotate =
         Matrix<float, 4>::rotation(y_angle, 0.0f, 1.0f, 0.0f);

      const Matrix<float, 4> compose = translate * rotate;

      for (size_t i = 0; i < (*it)->vertices.size(); i++) {
         const Vertex& v = (*it)->vertices[i];
         const Normal& n = (*it)->normals[i];

         target_chunk->vertices.push_back(compose.transform(v));
         target_chunk->normals.push_back(rotate.transform(n).normalise());

         const TexCoord& tc = (*it)->tex_coords[i];
         target_chunk->tex_coords.push_back(tc);

         const Colour& c = (*it)->colours[i];
         target_chunk->colours.push_back(c);
      }

      for (size_t i = 0; i < (*it)->indices.size(); i++) {
         Index orig = (*it)->indices[i];
         Index merged = orig + ibase;

         assert(orig < (*it)->vertices.size());
         assert(merged < target_chunk->vertices.size());

         target_chunk->indices.push_back(merged);
      }
   }

   reused += obuf.reused;
}

void MeshBuffer::print_stats() const
{
   debug() << "Mesh: " << vertex_count() << " vertices, "
	   << reused << " reused; "
           << chunks.size() << " chunks";
}

void MeshBuffer::add(const Vertex& vertex,
                     const Normal& normal,
                     const Colour& colour,
                     const TexCoord& a_tex_coord)
{
   if (!active_chunk) {
      // Create an initial chunk for the null texture
      bind(ITexturePtr());
   }

   // See if this vertex has already been added
   for (vector<Index>::iterator it = active_chunk->indices.begin();
	it != active_chunk->indices.end(); ++it) {
      if (vertex == active_chunk->vertices[*it]
          && normal ==  active_chunk->normals[*it]) {

	 const TexCoord& tc = active_chunk->tex_coords[*it];
         const bool same_tc = (approx_equal(tc.x, a_tex_coord.x)
                               && approx_equal(tc.y, a_tex_coord.y));

         const Colour& c = active_chunk->colours[*it];
	 const bool same_col = (approx_equal(c.r, colour.r)
                                && approx_equal(c.g, colour.g)
                                && approx_equal(c.b, colour.b));

         if (same_col && same_tc) {
            active_chunk->indices.push_back(*it);
	    reused++;
	    return;
         }
      }
   }

   const size_t index = active_chunk->vertices.size();
   active_chunk->vertices.push_back(vertex);
   active_chunk->normals.push_back(normal);
   active_chunk->tex_coords.push_back(a_tex_coord);
   active_chunk->colours.push_back(colour);
   active_chunk->indices.push_back(index);
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
   size_t offset = 0;

   for (vector<MeshBuffer::ChunkPtr>::const_iterator it = buf->chunks.begin();
        it != buf->chunks.end(); ++it) {

      for (size_t i = 0; i < (*it)->vertices.size(); i++) {
         VertexData* vd = &vertex_data[offset + i];

         vd->x = (*it)->vertices[i].x;
         vd->y = (*it)->vertices[i].y;
         vd->z = (*it)->vertices[i].z;

         vd->nx = (*it)->normals[i].x;
         vd->ny = (*it)->normals[i].y;
         vd->nz = (*it)->normals[i].z;

         if ((*it)->texture) {
            vd->tx = (*it)->tex_coords[i].x;
            vd->ty = 1.0f - (*it)->tex_coords[i].y;
         }

         vd->r = (*it)->colours[i].r;
         vd->g = (*it)->colours[i].g;
         vd->b = (*it)->colours[i].b;
      }

      offset += (*it)->vertices.size();
   }
}

// Inputs to glDrawRangeElements
struct ChunkDelim {
   ITexturePtr texture;
   GLsizei min, max;
   size_t offset, count;
};

static void copy_index_data(const MeshBuffer *buf,
                            vector<ChunkDelim>& delims,
                            GLushort *index_data)
{
   size_t global_idx = 0;
   GLushort offset = 0;

   vector<MeshBuffer::ChunkPtr>::const_iterator chunk_it;

   for (chunk_it = buf->chunks.begin();
        chunk_it != buf->chunks.end(); ++chunk_it) {

      ChunkDelim delim;
      delim.texture = (*chunk_it)->texture;
      delim.count = (*chunk_it)->indices.size();
      delim.offset = global_idx;

      vector<IMeshBuffer::Index>::const_iterator it;
      for (it = (*chunk_it)->indices.begin();
           it != (*chunk_it)->indices.end(); ++it) {

         index_data[global_idx++] = *it + offset;
      }

      delim.min = offset;

      offset += (*chunk_it)->vertices.size();

      delim.max = offset - 1;

      delims.push_back(delim);
   }
}

// Implementation of meshes using client side vertex arrays
class VertexArrayMesh : public IMesh {
public:
   VertexArrayMesh(IMeshBufferPtr a_buffer);
   ~VertexArrayMesh();

   void render() const;

private:
   size_t my_vertex_count;
   VertexData* my_vertex_data;
   size_t my_index_count;
   GLushort* my_indices;
   vector<ChunkDelim> chunks;
};

VertexArrayMesh::VertexArrayMesh(IMeshBufferPtr a_buffer)
{
   const MeshBuffer* buf = MeshBuffer::get(a_buffer);

   my_index_count = buf->index_count();
   my_indices = new GLushort[my_index_count];

   my_vertex_count = buf->vertex_count();
   my_vertex_data = new VertexData[my_vertex_count];

   copy_vertex_data(buf, my_vertex_data);
   copy_index_data(buf, chunks, my_indices);
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

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData),
                     reinterpret_cast<GLvoid*>(&my_vertex_data->tx));

   glEnableClientState(GL_COLOR_ARRAY);
   glColorPointer(3, GL_FLOAT, sizeof(VertexData),
                  reinterpret_cast<GLvoid*>(&my_vertex_data->r));

   glEnableClientState(GL_VERTEX_ARRAY);
   glEnableClientState(GL_NORMAL_ARRAY);
   glVertexPointer(3, GL_FLOAT, sizeof(VertexData),
                   reinterpret_cast<GLvoid*>(my_vertex_data));
   glNormalPointer(GL_FLOAT, sizeof(VertexData),
                   reinterpret_cast<GLvoid*>(&my_vertex_data->nx));

   glEnable(GL_COLOR_MATERIAL);

   for (vector<ChunkDelim>::const_iterator it = chunks.begin();
        it != chunks.end(); ++it) {

      if ((*it).texture) {
         glEnable(GL_TEXTURE_2D);
         (*it).texture->bind();

      }
      else {
         glDisable(GL_TEXTURE_2D);
      }

      glDrawRangeElements(GL_TRIANGLES,
                          (*it).min,
                          (*it).max,
                          (*it).count,
                          GL_UNSIGNED_SHORT,
                          my_indices + (*it).offset);
   }

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
   size_t index_count;
   vector<ChunkDelim> chunks;
};

VBOMesh::VBOMesh(IMeshBufferPtr a_buffer)
{
   // Get the data out of the buffer;
   const MeshBuffer* buf = MeshBuffer::get(a_buffer);

   const size_t vertex_count = buf->vertex_count();
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
   index_count = buf->index_count();
   GLushort* p_indices = new GLushort[index_count];

   copy_index_data(buf, chunks, p_indices);

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

   glEnable(GL_COLOR_MATERIAL);
   glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

   if (glIsEnabled(GL_BLEND))
      glDisable(GL_BLEND);

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

   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
   glTexCoordPointer(2, GL_FLOAT, sizeof(VertexData),
                     reinterpret_cast<GLvoid*>(offsetof(VertexData, tx)));

   for (vector<ChunkDelim>::const_iterator it = chunks.begin();
        it != chunks.end(); ++it) {

      if ((*it).count == 0)
         continue;

      if ((*it).texture) {
         glEnable(GL_TEXTURE_2D);
         (*it).texture->bind();
      }
      else {
         glDisable(GL_TEXTURE_2D);
      }

      const size_t offset_ptr = (*it).offset * sizeof(GLushort);

      glDrawRangeElements(GL_TRIANGLES,
                          (*it).min,
                          (*it).max,
                          (*it).count,
                          GL_UNSIGNED_SHORT,
                          reinterpret_cast<GLvoid*>(offset_ptr));
   }

   glPopClientAttrib();
   glPopAttrib();

   ::triangle_count += index_count / 3;
}

IMeshPtr make_mesh(IMeshBufferPtr buffer)
{
   //buffer->print_stats();

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
