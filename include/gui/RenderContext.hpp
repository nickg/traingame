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

#ifndef INC_GUI_RENDERCONTEXT_HPP
#define INC_GUI_RENDERCONTEXT_HPP

// Internal header: do not include this file directly

#include "Platform.hpp"
#include "Colour.hpp"
#include "ITexture.hpp"
#include "gui/Theme.hpp"

#include <string>
#include <stack>

namespace gui {

   class Widget;

   class RenderContext {
   public:
      RenderContext(const Theme& theme);
      ~RenderContext();

      void pushOrigin(const Widget* w);
      void popOrigin();

      void scissor(Widget* w);
      
      void rectangle(int x, int y, int w, int h, Colour c);
      void border(int x, int y, int w, int h, Colour c);
      void image(int x, int y, int w, int h, ITexturePtr tex);

      void print(IFontPtr font, int x, int y, const string& s,
         Colour col = colour::WHITE);

      const Theme& theme() const { return theme_; }
      
      void offset(int& x, int& y) const;

   private:
      const Theme& theme_;
      int origin_x, origin_y;

      stack<const Widget*> origin_stack;
   };
   
}

#endif
