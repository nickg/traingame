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

   // Mixin to add hide/show ability
   template <class Base>
   class Hideable : public Base {
   public:
      template <typename... Args>
      Hideable(const Args&&... args)
         : Base(args...), amVisible(true) {}

      virtual ~Hideable() {}
      
      void setVisible(bool visible)
      {
         amVisible = visible;
      }

      void render(int x, int y) const
      {
         if (amVisible)
            Base::render(x, y);
      }
   private:
      bool amVisible;
   };

   // Mixin to provide default event handlers
   template <class Base>
   class Defaults : public Base {
   public:
      template <typename... Args>
      Defaults(const Args&&... args)
         : Base(args...) {}
      
      void handleClick(int x, int y) {}
   };
   
}

#endif
