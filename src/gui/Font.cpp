//
//  Copyright (C) 2006-2009  Nick Gasson
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

#include "gui/IFont.hpp"
#include "ILogger.hpp"

#include <vector>
#include <stdexcept>
#include <cstdarg>

#include <GL/gl.h>
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

using namespace std;
using namespace gui;

// Concrete implementation of fonts
// This is based on a FreeType wrapper developed for my Lunar Lander game
class Font : public IFont {
public:
   Font(const string& aFile, int aHeight, bool shadow);
   ~Font();

   void print(int x, int y, const char* fmt, ...) const;
   int stringWidth(const char* fmt, ...) const;
   void setColour(float r, float g, float b, float a);
   int maxHeight() const { return myHeight; }
private:
   int nextPowerOf2(int a);
   void makeDisplayList(FT_Face face, char ch, GLuint listBase,
                        GLuint* texBase, unsigned short* widths, bool shadow);
   void splitIntoLines(vector<string> &lines, const char* fmt, va_list ap) const;

   float myR, myG, myB, myA;
   
   GLuint* textures;
   GLuint listBase;
   float height;
   unsigned short* widths;
   char* buf;
   int myHeight;

   static const int MAX_TXT_BUF = 1024;
   
   static int fontRefCount;
   static FT_Library library;
};

int Font::fontRefCount = 0;
FT_Library Font::library;

Font::Font(const string& aFile, int aHeight, bool shadow)
   : myR(1.0f), myG(1.0f), myB(1.0f), myA(1.0f), myHeight(aHeight)
{
   if (++fontRefCount == 1) {
      if (FT_Init_FreeType(&library))
         throw runtime_error("FT_Init_FreeType failed");
   }

   unsigned char i;

   buf = new char[MAX_TXT_BUF];

   // Allocate memory for textures
   textures = new GLuint[128];
   height = static_cast<float>(aHeight);
   widths = new unsigned short[128];

   // Create the face
   FT_Face face;
   if (FT_New_Face(library, aFile.c_str(), 0, &face))
      throw runtime_error("FT_New_Face failed, file name: " + aFile);

   // FreeType measures font sizes in 1/64ths of a pixel...
   FT_Set_Char_Size(face, aHeight<<6, aHeight<<6, 96, 96);

   listBase = glGenLists(128);
   glGenTextures(128, textures);

   // Generate the characters
   for (i = 0; i < 128; i++)
      makeDisplayList(face, i, listBase, textures, widths, shadow);

   // Free face data
   FT_Done_Face(face);

   log() << "Loaded font " << aFile;
}

Font::~Font()
{   
   glDeleteLists(listBase, 128);
   glDeleteTextures(128, textures);
   delete[] textures;
   delete[] widths;
   delete[] buf;
   
   if (--fontRefCount == 0)
      FT_Done_FreeType(library);
}

void Font::setColour(float r, float g, float b, float a)
{
   myR = r;
   myG = g;
   myB = b;
   myA = a;
}

int Font::nextPowerOf2(int a)
{
   int rval = 1;
   while (rval < a) 
      rval <<= 1;

   return rval;
}

void Font::makeDisplayList(FT_Face face, char ch, GLuint listBase,
                           GLuint* texBase, unsigned short* widths,
                           bool shadow)
{
   // Load the character's glyph
   if (FT_Load_Glyph(face, FT_Get_Char_Index(face, ch), FT_LOAD_DEFAULT))
      throw runtime_error("FT_Load_Glyph failed");

   // Store the face's glyph in a glyph object
   FT_Glyph glyph;
   if (FT_Get_Glyph(face->glyph, &glyph))
      throw runtime_error("FT_Get_Glyph failed");

   // Convert the glyph to a bitmap
   FT_Glyph_To_Bitmap(&glyph, ft_render_mode_normal, 0, 1);
   FT_BitmapGlyph bitmapGlyph = (FT_BitmapGlyph)glyph;

   // Get a reference to the bitmap
   FT_Bitmap& bitmap = bitmapGlyph->bitmap;

   // Make the width and height a power of 2
   int width = nextPowerOf2(bitmap.width);
   int height = nextPowerOf2(bitmap.rows);
	
   // Allocate memory for the texture data
   GLubyte* expandedData = new GLubyte[2 * width * height];

   // Fill in the bitmap's extended data
   for (int j = 0; j < height; j++) {
      for (int i = 0; i < width; i++) {
         expandedData[2*(i+j*width)] = expandedData[2*(i+j*width)+1] = 
            (i >= bitmap.width || j >= bitmap.rows)
            ? 0
            : bitmap.buffer[i + bitmap.width*j];
      }
   }
	
   // Set texture parameters
   glBindTexture(GL_TEXTURE_2D, texBase[(int)ch]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   // Create the texture
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, expandedData);

   // Free expanded data
   delete[] expandedData;

   // Create the display list
   glNewList(listBase+ch, GL_COMPILE);
   glBindTexture(GL_TEXTURE_2D, texBase[(int)ch]);

   glPushMatrix();

   // Insert some space between characters
   glTranslatef((float)bitmapGlyph->left, 0, 0);
   widths[(int)ch] = bitmapGlyph->left;

   // Move down a bit to accomodate characters such as p and q
   glTranslatef(0, (float)(-bitmapGlyph->top), 0);

   float x = (float)bitmap.width / (float)width;
   float y = (float)bitmap.rows / (float)height;

   // Draw the drop shadow if we want one
   if (shadow) {
      glPushAttrib(GL_CURRENT_BIT);
      glPushMatrix();
      
      glTranslatef(1.5f, 1.5f, 0.0f);
      glColor4f(0.0f, 0.0f, 0.0f, myA);
      
      glBegin(GL_QUADS);
      glTexCoord2f(0, y); glVertex2f(0, (float)bitmap.rows);
      glTexCoord2f(0, 0); glVertex2f(0, 0);
      glTexCoord2f(x, 0); glVertex2f((float)bitmap.width, 0);
      glTexCoord2f(x, y); glVertex2f((float)bitmap.width, (float)bitmap.rows);
      glEnd();

      glPopMatrix();
      glPopAttrib();
   }

   // Draw the quad
   glBegin(GL_QUADS);
   glTexCoord2f(0, y); glVertex2f(0, (float)bitmap.rows);
   glTexCoord2f(0, 0); glVertex2f(0, 0);
   glTexCoord2f(x, 0); glVertex2f((float)bitmap.width, 0);
   glTexCoord2f(x, y); glVertex2f((float)bitmap.width, (float)bitmap.rows);
   glEnd();
   
   glPopMatrix();

   // Move along to the next character
   glTranslatef((float)(face->glyph->advance.x >> 6), 0, 0);
   widths[(int)ch] += (short)face->glyph->advance.x >> 6;

   // Finish display list
   glEndList();
}

void Font::splitIntoLines(vector<string> &lines, const char* fmt, va_list ap) const
{
   if (fmt == NULL)
      *buf = 0;
   else
      vsnprintf(buf, MAX_TXT_BUF, fmt, ap);
   
   const char* start_line = buf, *c;
   for (c = buf; *c; c++)	{
      if (*c == '\n')	{
         string line;
         for (const char* n = start_line; n < c; n++) 
            line.append(1, *n);
         lines.push_back(line);
         start_line = c+1;
      }
   }

   if (start_line)	{
      string line;
      for (const char* n = start_line; n < c; n++) 
         line.append(1, *n);
      lines.push_back(line);
   }
}

void Font::print(int x, int y, const char* fmt, ...) const
{
   glPushAttrib(GL_ENABLE_BIT);

   GLuint font = listBase;
   float h = height / 0.63f;		// Add some space between lines  

   vector<string> lines;
   va_list ap;
   va_start(ap, fmt);
   splitIntoLines(lines, fmt, ap);
   va_end(ap);

   // Set attributes
   glEnable(GL_TEXTURE_2D);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glListBase(font);

   glColor4f(myR, myG, myB, myA);

   // Draw the text
   for (unsigned int i = 0; i < lines.size(); i++)	{
      glPushMatrix();
      glLoadIdentity();
      glTranslatef((float)x, (float)y - h*i + myHeight, 0.0f);

      glCallLists((GLsizei)lines[i].length(), GL_UNSIGNED_BYTE, lines[i].c_str());

      glPopMatrix();
   }

   glPopAttrib();
}

int Font::stringWidth(const char* fmt, ...) const
{
   va_list ap;
   vector<string> lines;

   va_start(ap, fmt);
   splitIntoLines(lines, fmt, ap);
   va_end(ap);

   int maxlen = 0;
   vector<string>::iterator it;
   for (it = lines.begin(); it != lines.end(); ++it) {
      int len = 0;
      for (string::iterator ch = (*it).begin(); ch != (*it).end(); ++ch)
         len += widths[(int)*ch];
      
      if (len > maxlen)
         maxlen = len;
   }

   return maxlen;
}

IFontPtr gui::loadFont(const string& aFile, int aHeight, bool shadow)
{
   return IFontPtr(new Font(aFile, aHeight, shadow));
}
