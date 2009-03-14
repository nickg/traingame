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

#include "ITextureManager.hpp"
#include "ILogger.hpp"

#include <map>
#include <sstream>
#include <stdexcept>

#include <GL/gl.h>
#include <SDL.h>
#include <SDL_image.h>

using namespace std;
using namespace std::tr1;

class Texture : public ITexture {
public:
   Texture(const string &file);
   virtual ~Texture();

   GLuint texture() const { return myTexture; }
   void bind() const;
   
   int width() const { return myWidth; }
   int height() const { return myHeight; }
   
private:
   GLuint myTexture;
   int myWidth, myHeight;
   
   static bool isPowerOfTwo(int n);
   static bool isTextureSizeSupported(int width, int height,
                                      int ncols = 4, GLenum format = GL_RGBA);
};

// Concrete implementation of ITextureManager
class TextureManager : public ITextureManager {
public:
   ITexturePtr load(const string& fileName);
private:
   map<string, ITexturePtr> myLoadedTextures;
};

ITexturePtr TextureManager::load(const string& fileName)
{
   map<string, ITexturePtr>::iterator it =
      myLoadedTextures.find(fileName);

   if (it != myLoadedTextures.end())
      return (*it).second;
   else {
      ITexturePtr ptr(new Texture(fileName));
      myLoadedTextures[fileName] = ptr;
      return ptr;
   }      
}

Texture::Texture(const string &file)
{
   SDL_Surface *surface = IMG_Load(file.c_str());
   if (NULL == surface) {
      ostringstream os;
      os << "Failed to load image: " << IMG_GetError();
      throw runtime_error(os.str());
   }

   if (!isPowerOfTwo(surface->w))
      warn() << file << " width not a power of 2";
   if (!isPowerOfTwo(surface->h))
      warn() << file << " height not a power of 2";

   if (!isTextureSizeSupported(surface->w, surface->h))
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

   myWidth = surface->w;
   myHeight = surface->h;
   
   glGenTextures(1, &myTexture);
   glBindTexture(GL_TEXTURE_2D, myTexture);

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   // Load the surface's data into the texture
   glTexImage2D(GL_TEXTURE_2D, 0, ncols, surface->w, surface->h, 0,
                texture_format, GL_UNSIGNED_BYTE, surface->pixels);

   SDL_FreeSurface(surface);
}

Texture::~Texture()
{
   glDeleteTextures(1, &myTexture);
}

bool Texture::isPowerOfTwo(int n)
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

bool Texture::isTextureSizeSupported(int width, int height, int ncols, GLenum format)
{
   glTexImage2D(GL_PROXY_TEXTURE_2D, 0, ncols, width, height, 0, format,
                GL_UNSIGNED_BYTE, NULL);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height);

   return height != 0 && width != 0;
}

void Texture::bind() const
{
   glBindTexture(GL_TEXTURE_2D, myTexture);
}

// Return the single instance of TextureManager
ITextureManagerPtr getTextureManager()
{
   static ITextureManagerPtr ptr(new TextureManager);
   return ptr;
}
