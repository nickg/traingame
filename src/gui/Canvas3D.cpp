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

#include "gui/Canvas3D.hpp"
#include "ILogger.hpp"
#include "GameScreens.hpp"

#include <GL/gl.h>
#include <GL/glu.h>

#include <boost/lexical_cast.hpp>

using namespace gui;
using namespace boost;

Canvas3D::Canvas3D(const AttributeSet& attrs)
   : Widget(attrs),
     clear(attrs.get<bool>("clear", true))
{

}

void Canvas3D::render(RenderContext& rc) const
{
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glPushMatrix();

   int xo = x(), yo = y();
   rc.offset(xo, yo);
   glViewport(xo, getGameWindow()->height() - yo - height(),
      width(), height());
   
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   
   const GLfloat wf = static_cast<GLfloat>(width());
   const GLfloat hf = static_cast<GLfloat>(height());
   gluPerspective(45.0f, wf/hf, 0.1f, 50.0f);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   if (clear)
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_LIGHTING);
   glEnable(GL_DEPTH_TEST);

   const_cast<Canvas3D*>(this)->raise(SIG_RENDER);
   
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();

   glMatrixMode(GL_MODELVIEW);
   glPopMatrix();
   
   glPopAttrib();
   
   GLenum error = glGetError();
   if (error != GL_NO_ERROR) {   
      throw runtime_error
         ("OpenGL error: " + lexical_cast<string>(gluErrorString(error)));
   }
}

      
