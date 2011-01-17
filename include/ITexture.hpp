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

#ifndef INC_ITEXTURE_HPP
#define INC_ITEXTURE_HPP

#include "Platform.hpp"
#include "IResource.hpp"

#include <string>

// Abstract interface to textures
struct ITexture {
   virtual ~ITexture() {}

   // Bind this texture to the current OpenGL texture
   virtual void bind() = 0;
   
   virtual int width() const = 0;
   virtual int height() const = 0;
};

typedef shared_ptr<ITexture> ITexturePtr;

// Load a texture and return a pointer to
// A texture will only be loaded at most once no matter how many
// times this is called
ITexturePtr load_texture(const string& a_file_name);

// Load a texture from a resource
ITexturePtr load_texture(IResourcePtr a_res, const string& a_file_name);

// Generate Perlin noise
ITexturePtr make_noise_texture(int size, int resolution, int base, int range);

#endif
