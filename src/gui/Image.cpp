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

#include "gui/IImage.hpp"
#include "ITexture.hpp"

#include <GL/gl.h>

using namespace gui;
using namespace std;

// Concrete implementaton of images
class Image : public IImage {
public:
   Image(const string& aFile);
   ~Image() {}

   void render(int x, int y) const;
private:
   ITexturePtr myTexture;
};

Image::Image(const string& aFile)
{
   myTexture = loadTexture(aFile);
}

void Image::render(int x, int y) const
{
   glPushAttrib(GL_ENABLE_BIT);
   glEnable(GL_TEXTURE_2D);
   
   myTexture->bind();

   const int w = myTexture->width();
   const int h = myTexture->height();

   glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
   
   glBegin(GL_QUADS);
   glTexCoord2i(0, 0);
   glVertex2i(x, y);
   glTexCoord2i(1, 0);
   glVertex2i(x + w, y);
   glTexCoord2i(1, 1);
   glVertex2i(x + w, y + h);
   glTexCoord2i(0, 1);
   glVertex2i(x, y + h);
   glEnd();

   glPopAttrib();
}

IImagePtr gui::makeImage(const string& aFile)
{
   return IImagePtr(new Image(aFile));
}
