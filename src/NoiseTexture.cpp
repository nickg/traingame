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
};

NoiseTexture::NoiseTexture(int size, int res)
   : size(size), resolution(res)
{
   GLubyte* pixels = new GLubyte[res * res];

   const int range = 15;

   const float step = float(size) / float(res);

   float xf, yf;
   int x, y;
   for (x = 0, xf = 0.0f; x < res; x++, xf += step) {
      for (y = 0, yf = 0.0f; y < res; y++, yf += step) {

         float freq = 1.0f;
         float sum = 0.0f;

         for (int i = 0; i < 4; i++) {
            sum += noise2d(xf * freq, yf * freq) / freq;
            freq *= 2.0f;
         }

         const int ni = min(255, max(0, 180 + int(float(range) * sum)));
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

   delete[] pixels;
}

NoiseTexture::~NoiseTexture()
{
   glDeleteTextures(1, &texture);
}

static inline float fade(float t)
{
   return t * t * t * (t * (t * 6 - 15) + 10);
}

static inline float lerp(float t, float a, float b)
{
   return a + t * (b - a);
}

static inline float grad(int hash, float x, float y, float z)
{
   int h = hash & 15;
   float u = h<8 ? x : y,
      v = h<4 ? y : h==12||h==14 ? x : z;
   return ((h&1) == 0 ? u : -u) + ((h&2) == 0 ? v : -v);
}

float NoiseTexture::noise2d(float x, float y) const
{
   // Based on reference implementation at http://mrl.nyu.edu/~perlin/noise/
   // with zero propagated through for z
   
   static const int p[512] = {
      151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,
      69,142, 8,99,37,240,21,10,23,190,
      6,148,247,120,234,75,0,26,197,62,94,252,219,203,
      117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,
      68,175,74,
      165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,
      105,92,41,55,46,245,40,244, 102,143,54, 65,25,63,161,
      1,216,80,73,209,76,132, 187,208, 89,18,169,200,196,
      135,130,116,188,159,86,164,100,109,198,173,186,
      3,64,52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,
      227,47,16,58,17,182,189,28,42,223,183,170,213,119,248,152,
      2,44,154,163, 70,221,153,101,155,167, 43,172,9,129,22,39,253,
      19,98,108,110,79,113,224, 232,178,185, 112,104,218,246,97,228,
      251,34,242,193,238,210,144,12,191,179,162,241,
      81,51,145,235,249,14,239,107, 49,192,214, 31,181,199,106,157,184,
      84,204,176,115,121,50,45,127, 4,150,254,
      138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
      151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,
      69,142, 8,99,37,240,21,10,23,190,
      6,148,247,120,234,75,0,26,197,62,94,252,219,203,
      117,35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,
      68,175,74, 165,71,134,139,48,27,166,
      77,146,158,231,83,111,229,122,60,211,133,230,220,
      105,92,41,55,46,245,40,244, 102,143,54, 65,25,63,161,
      1,216,80,73,209,76,132,187,208, 89,18,169,200,196,
      135,130,116,188,159,86,164,100,109,198,173,186,
      3,64,52,217,226,250,124,123,
      5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
      189,28,42,
      223,183,170,213,119,248,152, 2,44,154,163, 70,221,153,101,155,167,
      43,172,9, 129,22,39,253, 19,98,108,110,79,113,224,232,178,185,
      112,104,218,246,97,228,
      251,34,242,193,238,210,144,12,191,179,162,241,
      81,51,145,235,249,14,239,107, 49,192,214, 31,181,199,106,157,184,
      84,204,176,115,121,50,45,127, 4,150,254,
      138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
   };
   
   const int X = int(floorf(x)) & 255;
   const int Y = int(floorf(y)) & 255;
   
   x -= floorf(x);
   y -= floorf(y);
   
   const float u = fade(x);
   const float v = fade(y);
   const int A = p[X  ]+Y, AA = p[A], AB = p[A+1],
      B = p[X+1]+Y, BA = p[B], BB = p[B+1];

   return lerp(0.0f, lerp(v, lerp(u, grad(p[AA  ], x  , y  ,  0.0f ),
                                     grad(p[BA  ], x-1, y  ,  0.0f )),
                             lerp(u, grad(p[AB  ], x  , y-1,  0.0f ),
                                     grad(p[BB  ], x-1, y-1,  0.0f ))),
                     lerp(v, lerp(u, grad(p[AA+1], x  , y  , -1.0f ),
                                     grad(p[BA+1], x-1, y  , -1.0f )),
                             lerp(u, grad(p[AB+1], x  , y-1, -1.0f ),
                                     grad(p[BB+1], x-1, y-1, -1.0f ))));
}

void NoiseTexture::bind()
{
   glBindTexture(GL_TEXTURE_2D, texture);
}

ITexturePtr make_noise_texture(int size, int resolution)
{
   return ITexturePtr(new NoiseTexture(size, resolution));
}
