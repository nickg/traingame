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

#include <memory>
#include <tuple>

#include "Maths.hpp"
#include "ITexture.hpp"

// Represent OpenGL materials
struct Material {
   Material();
   
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
   typedef std::tuple<float, float, float> Colour;
   typedef unsigned Index;

   virtual ~IMeshBuffer() {}
   
   virtual void add(const Vertex& aVertex, const Normal& aNormal) = 0;
   virtual void add(const Vertex& aVertex, const Normal& aNormal,
                    const TexCoord& aTexCoord) = 0;
   virtual void add(const Vertex& aVertex, const Normal& aNormal,
                    const Colour& aColour) = 0;

   virtual void bindMaterial(const Material& aMaterial) = 0;
   
   virtual void printStats() const = 0;
};

typedef std::shared_ptr<IMeshBuffer> IMeshBufferPtr;

// Generic interface to meshes
struct IMesh {
   virtual ~IMesh() {}

   virtual void render() const = 0;
};

typedef std::shared_ptr<IMesh> IMeshPtr;

IMeshPtr makeMesh(IMeshBufferPtr aBuffer);
IMeshBufferPtr makeMeshBuffer();

#endif
