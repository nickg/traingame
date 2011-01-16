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
#include "Random.hpp"

#include <cstdlib> // XXX

class NoiseTexture : public ITexture {
public:
   NoiseTexture(int size, int res);
   ~NoiseTexture();

   // ITexture interface
   void bind();
   int width() const { return resolution; }
   int height() const { return resolution; }

private:
   void build_gradients();
   float noise2d(float x, float y) const;
   const VectorF& gradient(int x, int y) const;
   float ease_curve(float t) const;
   
   const int size, resolution;
   GLuint texture;
   VectorF *gradients;
};

NoiseTexture::NoiseTexture(int size, int res)
   : size(size), resolution(res), gradients(NULL)
{
   GLubyte* pixels = new GLubyte[res * res];

   const int range = 40;

   build_gradients();
   
   const float step = float(size) / float(res);

   float xf, yf;
   int x, y;
   for (x = 0, xf = 0.0f; x < res; x++, xf += step) {
      for (y = 0, yf = 0.0f; y < res; y++, yf += step) {

         float freq = 1.0f;
         float sum = 0.0f;

         for (int i = 0; i < 8; i++) {
            sum += noise2d(xf * freq, yf * freq) / freq;
            freq *= 2.0f;
         }

         const int ni = min(255, max(0, 150 + int(float(range) * sum)));
         pixels[x + (y * res)] = ni;
      }
   }

   glGenTextures(1, &texture);
   glBindTexture(GL_TEXTURE_2D, texture);

   // Use GL_NEAREST here for better performance
   // Or GL_LINEAR for better apppearance
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   // Load the surface's data into the texture
   glTexImage2D(GL_TEXTURE_2D, 0, 3, res, res, 0,
                GL_LUMINANCE, GL_UNSIGNED_BYTE, pixels);

   delete[] gradients;
   gradients = NULL;
   
   delete[] pixels;
}

NoiseTexture::~NoiseTexture()
{
   glDeleteTextures(1, &texture);
}

void NoiseTexture::build_gradients()
{
   static Uniform<float> rnd(0.1f, 1.0f);
   
   gradients = new VectorF[size * size];

   for (int x = 0; x < size; x++) {
      for (int y = 0; y < size; y++) {
         VectorF& v = gradients[x + y*size];
         
         v.x = rnd();
         v.y = rnd();
         v.z = 0.0f;

         v.normalise();
      }
   }
}

float NoiseTexture::ease_curve(float t) const
{
   assert(t >= 0.0f && t <= 1.0f);
   return (3 * t * t) - (2 * t * t * t);      
}

float NoiseTexture::noise2d(float x, float y) const
{
   // Perlin noise function
   
   const int ix = floorf(x);
   const int iy = floorf(y);

   const VectorF xy = make_vector(x, y, 0.0f);
   const VectorF x0y0 = make_vector(float(ix), float(iy), 0.0f);
   const VectorF x1y0 = make_vector(float(ix + 1), float(iy), 0.0f);
   const VectorF x0y1 = make_vector(float(ix), float(iy + 1), 0.0f);
   const VectorF x1y1 = make_vector(float(ix + 1), float(iy + 1), 0.0f);

   const float s = gradient(ix, iy).dot(xy - x0y0);
   const float t = gradient(ix + 1, iy).dot(xy - x1y0);
   const float u = gradient(ix, iy + 1).dot(xy - x0y1);
   const float v = gradient(ix + 1, iy + 1).dot(xy - x1y1);

   const float sx = ease_curve(x - float(ix));
   const float sy = ease_curve(y - float(iy));

   const float a = s + sx * (t - s);
   const float b = u + sx * (v - u);
   
   return (a + sy * (b - a));
}

const VectorF& NoiseTexture::gradient(int x, int y) const
{
   x = x % size;
   y = y % size;

   return gradients[x + (y * size)];
}

void NoiseTexture::bind()
{
   glBindTexture(GL_TEXTURE_2D, texture);
}

ITexturePtr make_noise_texture(int size, int resolution)
{
   return ITexturePtr(new NoiseTexture(size, resolution));
}
