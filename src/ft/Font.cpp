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

#include "ft/IFont.hpp"
#include "ILogger.hpp"

#include <map>
#include <stdexcept>
#include <vector>
#include <cassert>

#include <boost/lexical_cast.hpp>

#include <GL/gl.h>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

using namespace ft;

class Glyph {
public:
   Glyph(FT_Face& face, FT_ULong uch, FontType type);
   ~Glyph();

   void render() const;

   float width() const { return width_; }
   float height() const { return height_; }

   float advance_x() const { return advance_x_; }
   float advance_y() const { return advance_y_; }
private:
   static int next_power_of_2(int a);
   
   GLuint tex;
   float width_, height_;
   float tex_xmax, tex_ymax;
   float top, left;
   float advance_x_, advance_y_;
};

Glyph::Glyph(FT_Face& face, FT_ULong uch, FontType type)
{
   int index = FT_Get_Char_Index(face, uch);

   int err = FT_Load_Glyph(face, index, FT_LOAD_DEFAULT);
   if (err)
      throw runtime_error("FT_Load_Glyph failed for char code "
         + boost::lexical_cast<string>(uch));

   glGenTextures(1, &tex);
   
   glBindTexture(GL_TEXTURE_2D, tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      
   err = FT_Render_Glyph(face->glyph,
      type == FONT_NORMAL
      ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO);
   assert(err == 0);

   FT_Bitmap& bmp = face->glyph->bitmap;
      
   // Pad out the bitmap to be a power-of-2 square
      
   int tex_width = max(next_power_of_2(bmp.width), 2);
   int tex_height = max(next_power_of_2(bmp.rows), 2);

   GLubyte* expanded = new GLubyte[2 * tex_width * tex_height];

   if (type == FONT_NORMAL) {
      // Anti-aliased font
         
      for (int j = 0; j < tex_height; j++) {
         for (int i = 0; i < tex_width; i++) {
            expanded[2*(i + j*tex_width)]
               = expanded[2*(i + j*tex_width) + 1]
               = (i >= bmp.width || j >= bmp.rows)
               ? 0
               : bmp.buffer[i + bmp.width*j];
         }
      }
   }
   else {
      // Monochrome font
         
      int bmp_bit = 7, bmp_byte = 0;      
         
      for (int j = 0; j < tex_height; j++) {
         for (int i = 0; i < tex_width; i++) {
               
            int val;
            if (i >= bmp.width || j >= bmp.rows)
               val = 0;
            else
               val = bmp.buffer[bmp_byte + j*bmp.pitch] & (1<<bmp_bit);
               
            expanded[2*(i + j*tex_width)]
               = expanded[2*(i + j*tex_width) + 1]
               = (val ? 255 : 0);

            if (bmp_bit-- == 0) {
               bmp_bit = 7;
               bmp_byte++;
            }
         }

         bmp_bit = 7;
         bmp_byte = 0;
      }
   }
      
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width,
      tex_height, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, expanded);

   delete[] expanded;

   width_ = static_cast<float>(bmp.width);
   height_ = static_cast<float>(bmp.rows);   

   tex_xmax = width_ / tex_width;
   tex_ymax = height_ / tex_height;
      
   top = face->glyph->bitmap_top;
   left = face->glyph->bitmap_left;

   advance_x_ = face->glyph->advance.x >> 6;
   advance_y_ = face->glyph->advance.y >> 6;
}

Glyph::~Glyph()
{
   glDeleteTextures(1, &tex);
}

void Glyph::render() const
{
   glBindTexture(GL_TEXTURE_2D, tex);
   
   glBegin(GL_QUADS);
   
   glTexCoord2f(0, tex_ymax);
   glVertex2f(left, height_ - top);
   
   glTexCoord2f(0, 0);
   glVertex2f(left, -top);
   
   glTexCoord2f(tex_xmax, 0);
   glVertex2f(left + width_, -top);
   
   glTexCoord2f(tex_xmax, tex_ymax);
   glVertex2f(left + width_, height_ - top);

   glEnd();

   glTranslatef(advance_x_, advance_y_, 0.0f);
}

int Glyph::next_power_of_2(int a)
{
   int rval = 1;
   while (rval < a) 
      rval <<= 1;

   return rval;
}

namespace {// REMOVE

class Font : public IFont {
public:
   Font(const string& file, int h, FontType type);
   ~Font();
   
   void print(int x, int y, Colour c, const string& s) const;

   int height() const { return height_; }
   int text_width(const string& s) const;
private:
   FT_Face face;
   vector<Glyph*> glyphs;
   int height_;
   int line_skip_;
   
   static FT_Library library;
   static int library_ref_count;
};

FT_Library Font::library;
int Font::library_ref_count = 0;

Font::Font(const string& file, int h, FontType type)
{
   if (++library_ref_count == 1) {
      if (FT_Init_FreeType(&library))
         throw runtime_error("FT_Init_FreeType failed");
   }

   int err = FT_New_Face(library, file.c_str(), 0, &face);
   if (err == FT_Err_Unknown_File_Format)
      throw runtime_error("Unsupported font file format: " + file);
   else if (err)
      throw runtime_error("Failed to load font: " + file);

   err = FT_Set_Char_Size(face,
      0,      // Width in 1/64th of point
      h*64,   // Height in 1/64th of point
      0, 0);  // Default DPI

   for (char ch = 0; ch < 127; ch++)
      glyphs.push_back(new Glyph(face, ch, type));
   
   int ascent  = static_cast<int>(face->size->metrics.ascender / 64.0);
   int descent  = static_cast<int>(face->size->metrics.descender / 64.0);
   height_ = ascent - descent + 1;

   line_skip_ = static_cast<int>(face->size->metrics.height / 64.0); 
    
   log() << "Loaded font " << file;
}

Font::~Font()
{
   for (vector<Glyph*>::iterator it = glyphs.begin();
        it != glyphs.end(); ++it)
      delete *it;

   FT_Done_Face(face);
   
   if (--library_ref_count == 0)
      FT_Done_FreeType(library);
}

void Font::print(int x, int y, Colour c, const string& s) const
{
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   
   glPushMatrix();

   glColor4f(get<0>(c), get<1>(c), get<2>(c), get<3>(c));
   glTranslatef(x, y + (face->size->metrics.ascender>>6), 0.0f);

   for (string::const_iterator it = s.begin();
        it != s.end(); ++it)
      glyphs.at(*it)->render();

   glPopMatrix();
   glPopAttrib();
}

int Font::text_width(const string& s) const
{
   float w = 0.0f;
   for (string::const_iterator it = s.begin();
        it != s.end(); ++it)
      w += glyphs.at(*it)->advance_x();

   return static_cast<int>(w);
}

}
IFontPtr ft::load_font(const string& file, int h, FontType type)
{
   typedef tuple<string, int> FontToken;
   static map<FontToken, IFontPtr> cache;

   FontToken t = make_tuple(file, h);
   map<FontToken, IFontPtr>::iterator it = cache.find(t);

   if (it != cache.end())
      return (*it).second;
   else {
      IFontPtr p(new Font(file, h, type));
      cache[t] = p;
      return p;
   }
}


