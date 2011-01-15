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

#ifndef INC_IMESH_HPP
#define INC_IMESH_HPP

#include "Platform.hpp"
#include "Maths.hpp"
#include "ITexture.hpp"
#include "Colour.hpp"

// Represent OpenGL materials
struct Material {
   Material();

   void apply() const;
   
   float diffuseR, diffuseG, diffuseB;
   float ambientR, ambientG, ambientB;
   float specularR, specularG, specularB;
   ITexturePtr texture;
};

// Represents the vertices, normals, etc. in a mesh
struct IMeshBuffer {
   typedef Vector<float> Vertex;
   typedef Vector<float> Normal;
   typedef Point<float> TexCoord;
   typedef unsigned Index;

   virtual ~IMeshBuffer() {}

   virtual size_t vertex_count() const = 0;
   
   virtual void add(const Vertex& vertex, const Normal& normal) = 0;
   virtual void add(const Vertex& vertex, const Normal& normal,
      const TexCoord& a_tex_coord) = 0;
   virtual void add(const Vertex& vertex, const Normal& normal,
      const Colour& colour) = 0;

   // Convenience functions
   virtual void add_quad(Vertex a, Vertex b, Vertex c, Vertex d,
      Colour colour) = 0;
   virtual void add_quad(Vertex a, Vertex b, Vertex c, Vertex d,
      Normal na, Normal nb, Normal nc, Normal nd,
      Colour colour) = 0;

   virtual void bind_material(const Material& a_material) = 0;
   
   virtual void print_stats() const = 0;

   virtual void merge(shared_ptr<IMeshBuffer> other,
      Vector<float> off, float y_angle=0.0f) = 0;
};

typedef shared_ptr<IMeshBuffer> IMeshBufferPtr;

// Generic interface to meshes
struct IMesh {
   virtual ~IMesh() {}

   virtual void render() const = 0;
};

typedef shared_ptr<IMesh> IMeshPtr;

IMeshPtr make_mesh(IMeshBufferPtr a_buffer);
IMeshBufferPtr make_mesh_buffer();
void update_render_stats();
int get_average_triangle_count();

#endif
