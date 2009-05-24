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
#include "gui/Internal.hpp"

#include <boost/signals.hpp>

using namespace gui;
using namespace std;

// Concrete implementation of push buttons
class Button : public IButton {
public:
   Button(const string& aGlyphFile);
   virtual ~Button() {}

   // IControl interface
   int width() const;
   int height() const;
   void render(int x, int y) const;
   bool handleClick(int x, int y);
   bool handleMouseRelease(int x, int y);

   // IButton interface
   void onClick(ClickHandler aHandler);
private:
   IImagePtr myGlyphImage;
   boost::signal<void (void)> myClickSignal;
   bool amActive;
   
   static IImagePtr ourBaseImage, ourActiveImage;
};

IImagePtr Button::ourBaseImage, Button::ourActiveImage;

Button::Button(const string& aGlyphFile)
   : amActive(false)
{
   if (!ourBaseImage)
      ourBaseImage = makeImage("data/images/button_base.png");

   if (!ourActiveImage)
      ourActiveImage = makeImage("data/images/button_active.png");

   myGlyphImage = makeImage(aGlyphFile);
}

void Button::onClick(ClickHandler aHandler)
{
   myClickSignal.connect(aHandler);
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
   if (amActive)
      ourActiveImage->render(x, y);
   else
      ourBaseImage->render(x, y);
   myGlyphImage->render(x, y);
}

bool Button::handleClick(int x, int y)
{
   amActive = true;
   myClickSignal();
   return true;
}

bool Button::handleMouseRelease(int x, int y)
{
   amActive = false;
   return true;
}

IButtonPtr gui::makeButton(const string& aGlyphFile)
{
   return IButtonPtr
      (new Moveable<Hideable<Button>>(aGlyphFile));
}

