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

#ifndef INC_INTERNAL_HPP
#define INC_INTERNAL_HPP

#include "IControl.hpp"

// Common implementation details for GUI components
namespace gui {

   // Default implementations of some IControl methods
   class ControlImpl : virtual public IControl {
   public:
      ControlImpl() : amVisible(true) {}
      
      void setVisible(bool visible)
      {
         amVisible = visible;
      }

      virtual void render(int x, int y) const
      {
         if (amVisible)
            renderVisible(x, y);
      }

      virtual void renderVisible(int x, int y) const = 0;

      void handleClick(int x, int y) {}
      
   protected:
      bool amVisible;
   };
   
}

#endif
