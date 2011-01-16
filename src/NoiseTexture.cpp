//
//  Copyright (C) 2011  Nick Gasson
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
#include "OpenGLHelper.hpp"

#include <cstdlib> // XXX

class NoiseTexture : public ITexture {
public:
   NoiseTexture(int w, int h);
   ~NoiseTexture();

   // ITexture interface
   void bind();
   int width() const { return width_; }
   int height() const { return height_; }

private:
   const int width_, height_;
   GLuint texture;
};

NoiseTexture::NoiseTexture(int w, int h)
   : width_(w), height_(h)
{
   GLubyte* pixels = new GLubyte[w * h];

   for (int x = 0; x < w; x++) {
      for (int y = 0; y < h; y++)
         pixels[x + (y*w)] = 127 + (random() % 32) - 16;
   }
   
   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);

   // Use GL_NEAREST here for better performance
   // Or GL_LINEAR for better apppearance
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   // Load the surface's data into the texture
   glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0,
                GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);

   delete[] pixels;
}

NoiseTexture::~NoiseTexture()
{
   glDeleteTextures(1, &texture);
}

void NoiseTexture::bind()
{
   glBindTexture(GL_TEXTURE_2D, texture);
}

ITexturePtr make_noise_texture(int width, int height)
{
   return ITexturePtr(new NoiseTexture(width, height));
}
