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

#include "gui/IControl.hpp"
#include "gui/IImage.hpp"

using namespace gui;
using namespace std;

// Concrete implementation of push buttons
class Button : public IControl {
public:
   Button(const string& aGlyphFile);
   ~Button() {}

   // IControl interace
   void render(int x, int y) const;
   int width() const;
   int height() const;
   void setVisible(bool visible) { amVisible = visible; } 
private:
   IImagePtr myGlyphImage;
   bool amVisible;
   
   static IImagePtr ourBaseImage, ourActiveImage;
};

IImagePtr Button::ourBaseImage, Button::ourActiveImage;

Button::Button(const string& aGlyphFile)
   : amVisible(true)
{
   if (!ourBaseImage)
      ourBaseImage = makeImage("data/images/button_base.png");

   if (!ourActiveImage)
      ourActiveImage = makeImage("data/images/button_active.png");

   myGlyphImage = makeImage(aGlyphFile);
}

int Button::width() const
{
   return myGlyphImage->width();
}

int Button::height() const
{
   return myGlyphImage->height();
}

void Button::render(int x, int y) const
{
   if (!amVisible)
      return;
   
   ourBaseImage->render(x, y);
   myGlyphImage->render(x, y);
}

IControlPtr gui::makeButton(const string& aGlyphFile)
{
   return IControlPtr(new Button(aGlyphFile));
}

