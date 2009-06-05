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

   // It would be nice if we could use variadic templates to
   // implement variable-arity generic constructors but these
   // are only supported in experimental GCC and not at all
   // in Visual C++

   // Mixin to add hide/show ability
   template <class Base>
   class Hideable : public Base {
   public:
      Hideable() : amVisible(true) {}
      
      template <typename A>
      Hideable(const A& a)
         : Base(a), amVisible(true) {}

      template <typename A, typename B>
      Hideable(const A& a, const B& b)
         : Base(a, b), amVisible(true) {}

      template <typename A, typename B, typename C>
      Hideable(const A& a, const B& b, const C& c)
         : Base(a, b, c), amVisible(true) {}
      
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
      Defaults() {}
      
      template <typename A>
      Defaults(const A& a)
         : Base(a) {}

      template <typename A, typename B>
      Defaults(const A& a, const B& b)
         : Base(a, b) {}

      template <typename A, typename B, typename C>
      Defaults(const A& a, const B& b, const C& c)
         : Base(a, b, c) {}
      
      virtual ~Defaults() {}
      
      bool handleClick(int x, int y) { return false; }
      bool handleMouseRelease(int x, int y) { return false; }
   };

   // Mixin to provide screen placement
   template <class Base>
   class Moveable : public Base {
   public:
      Moveable() : myX(0), myY(0) {}
      
      template <typename A>
      Moveable(const A& a)
         : Base(a), myX(0), myY(0) {}

      template <typename A, typename B>
      Moveable(const A& a, const B& b)
         : Base(a, b), myX(0), myY(0) {}

      template <typename A, typename B, typename C>
      Moveable(const A& a, const B& b, const C& c)
         : Base(a, b, c), myX(0), myY(0) {}

      virtual ~Moveable() {}

      void setOrigin(int x, int y) { myX = x; myY = y; }
      void origin(int& x, int& y) const { x = myX; y = myY; }
   private:
      int myX, myY;
   };
   
}

#endif
