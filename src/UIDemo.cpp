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

#include "IScreen.hpp"
#include "gui2/ILayout.hpp"
#include "ft/IFont.hpp"

class UIDemo : public IScreen {
public:
   UIDemo();

   // IScreen interface
   void display(IGraphicsPtr aContext) const {}
   void overlay() const;
   void update(IPickBufferPtr aPickBuffer, int aDelta) {}
   void onKeyDown(SDLKey aKey) {}
   void onKeyUp(SDLKey aKey) {}
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y,
      int xrel, int yrel) {}
   void onMouseClick(IPickBufferPtr pick_buffer, int x, int y,
      MouseButton button) {}
   void onMouseRelease(IPickBufferPtr pick_buffer, int x, int y,
      MouseButton button) {}
   
private:
   gui::ILayoutPtr layout;
   ft::IFontPtr font;
};

UIDemo::UIDemo()
{
   layout = gui::make_layout("layouts/demo.xml");
   font = ft::load_font("data/fonts/Vera.ttf", 16);
}

void UIDemo::overlay() const
{
   //layout->render();

   font->print(100, 100, "yah");
}

IScreenPtr make_ui_demo()
{
   return IScreenPtr(new UIDemo);
}
