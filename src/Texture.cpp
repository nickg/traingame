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

#include "ITexture.hpp"
#include "ILogger.hpp"

#include <map>
#include <sstream>
#include <stdexcept>

#include <GL/glew.h>
#include <GL/gl.h>
#include <SDL.h>
#include <SDL_image.h>

class Texture : public ITexture {
public:
   Texture(const string &file);
   ~Texture();

   GLuint texture() const { return my_texture; }
   void bind();

   int width() const { return my_width; }
   int height() const { return my_height; }

private:
   GLuint my_texture;
   int my_width, my_height;

   static bool is_power_ofTwo(int n);
   static bool is_texture_sizeSupported(int width, int height,
                                      int ncols = 4, GLenum format = GL_RGBA);
};

// Texture cache
namespace {
   map<string, ITexturePtr> the_texture_cache;
}

ITexturePtr load_texture(const string& a_file_name)
{
   map<string, ITexturePtr>::iterator it =
      the_texture_cache.find(a_file_name);

   if (it != the_texture_cache.end())
      return (*it).second;
   else {
      ITexturePtr ptr(new Texture(a_file_name));
      the_texture_cache[a_file_name] = ptr;
      return ptr;
   }
}

ITexturePtr load_texture(IResourcePtr a_res, const string& a_file_name)
{
   // Hack alert! Just use the handle to find out the file name
   // This should be replaced with a solution where all textures come
   // from resources...
   string real_file_name;
   {
      IResource::Handle h = a_res->open_file(a_file_name);
      real_file_name = h.file_name();
   } // Handle closed here

   return load_texture(real_file_name);
}

Texture::Texture(const string &file)
{
   SDL_Surface *surface = IMG_Load(file.c_str());
   if (NULL == surface) {
      ostringstream os;
      os << "Failed to load image: " << IMG_GetError();
      throw runtime_error(os.str());
   }

   if (!is_power_ofTwo(surface->w))
      warn() << file << " width not a power of 2";
   if (!is_power_ofTwo(surface->h))
      warn() << file << " height not a power of 2";

   if (!is_texture_sizeSupported(surface->w, surface->h))
      warn() << file << " bigger than max OpenGL texture";

   int ncols = surface->format->BytesPerPixel;
   GLenum texture_format;
   if (ncols == 4) {
      // Contains an alpha channel
      if (surface->format->Rmask == 0x000000ff)
         texture_format = GL_RGBA;
      else
         texture_format = GL_BGRA;
   }
   else if (ncols == 3) {
      if (surface->format->Rmask == 0x000000ff)
         texture_format = GL_RGB;
      else
         texture_format = GL_BGR;
   }
   else {
      ostringstream os;
      os << "Unsupported image colour format: " << file;
      throw runtime_error(os.str());
   }

   my_width = surface->w;
   my_height = surface->h;

   glGenTextures(1, &my_texture);
   glBindTexture(GL_TEXTURE_2D, my_texture);

   // Use GL_NEAREST here for better performance
   // Or GL_LINEAR for better apppearance
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   // Load the surface's data into the texture
   glTexImage2D(GL_TEXTURE_2D, 0, ncols, surface->w, surface->h, 0,
                texture_format, GL_UNSIGNED_BYTE, surface->pixels);

   SDL_FreeSurface(surface);

   log() << "Loaded texture " << file;
}

Texture::~Texture()
{
   glDeleteTextures(1, &my_texture);
}

bool Texture::is_power_ofTwo(int n)
{
   int pop = 0;
   for (unsigned i = 0, bit = 1;
        i < sizeof(int)*8;
        i++, bit <<= 1) {
      if (n & bit)
         pop++;
   }
   return pop == 1;
}

bool Texture::is_texture_sizeSupported(int width, int height, int ncols, GLenum format)
{
   glTexImage2D(GL_PROXY_TEXTURE_2D, 0, ncols, width, height, 0, format,
                GL_UNSIGNED_BYTE, NULL);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

   return height != 0 && width != 0;
}

void Texture::bind()
{
   glBindTexture(GL_TEXTURE_2D, my_texture);
}
