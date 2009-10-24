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

#include "gui/IContainer.hpp"
#include "gui/Internal.hpp"

#include <GL/gl.h>

using namespace gui;

// A window with a title bar
class Panel : public Defaults<Moveable<IContainer> > {
public:
   Panel(const string& aTitle, IContainerPtr aContent);
   ~Panel() {}

   // IControl interface
   int width() const;
   int height() const;
   void render(int x, int y) const;

   // IContainer interface
   void addChild(IControlPtr aControl) { myContent->addChild(aControl); }
private:
   string myTitle;
   int myTitleWidth;
   IFontPtr myFont;
   IContainerPtr myContent;

   static const int TITLE_PAD = 6;
};

Panel::Panel(const string& aTitle, IContainerPtr aContent)
   : myTitle(aTitle), myContent(aContent)
{
   myFont = load_font("data/fonts/Vera.ttf", 13, false);
   myFont->set_colour(0.0f, 0.0f, 0.0f);
   myTitleWidth = myFont->string_width(aTitle.c_str());
}

void Panel::render(int x, int y) const
{
   // Discard the given x and y and use the absolute position
   origin(x, y);

   const int w = width();
   const int h = height();

   glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
   glBegin(GL_QUADS);
   glVertex2i(x, y);
   glVertex2i(x + w, y);
   glVertex2i(x + w, y + h);
   glVertex2i(x, y + h);
   glEnd();

   myFont->print(x + TITLE_PAD, y + TITLE_PAD, myTitle.c_str());
}

int Panel::width() const
{
   return max(myTitleWidth + 2*TITLE_PAD, myContent->width());
}

int Panel::height() const
{
   return myFont->max_height() + 2*TITLE_PAD + myContent->height();
}

IContainerPtr gui::makePanel(const string& aTitle, IContainerPtr aContent)
{
   return IContainerPtr(new Hideable<Panel>(aTitle, aContent));
}
