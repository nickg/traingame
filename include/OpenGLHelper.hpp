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

#ifndef INC_OPENGL_HELPER_HPP
#define INC_OPENGL_HELPER_HPP

#include "Platform.hpp"
#include "IWindow.hpp"
#include "IGraphics.hpp"
#include "IPickBuffer.hpp"
#include "Colour.hpp"

#include <GL/gl.h>

// Helper functions used by the different IWindow implementations
void initGL();
void drawGLScene(IWindowPtr a_window, IGraphicsPtr a_context, IScreenPtr a_screen);
void resizeGLScene(IWindowPtr a_window);
void printGLVersion();
void checkGLError();

// Wrappers for OpenGL picking features
void begin_pick(IWindowPtr a_window, unsigned* a_buffer, int x, int y);
unsigned end_pick(unsigned* a_buffer);

// Helper functions for using our Vector and Colour objects
// as OpenGL types
namespace gl {

   inline void colour(const Colour& c)
   {
      glColor4f(c.r, c.g, c.b, c.a);
   }

   template <class T>
   inline void translate(const Vector<T>& v);

   template <>
   inline void translate(const Vector<float>& v)
   {
      glTranslatef(v.x, v.y, v.z);
   }

   template <class T>
   inline void vertex(const Vector<T>& v);

   template <>
   inline void vertex(const Vector<float>& v)
   {
      glVertex3f(v.x, v.y, v.z);
   }

   template <class T>
   inline void normal(const Vector<T>& v);

   template <>
   inline void normal(const Vector<float>& v)
   {
      glNormal3f(v.x, v.y, v.z);
   }
}

#endif
