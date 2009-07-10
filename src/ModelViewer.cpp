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

#include "ModelViewer.hpp"
#include "ILight.hpp"

#include <stdexcept>

#include <boost/lexical_cast.hpp>

#include <GL/gl.h>
#include <GL/glu.h>

using namespace boost;

ModelViewer::ModelViewer(int x, int y, int w, int h)
   : Fl_Gl_Window(x, y, w, h)
{

}

ModelViewer::~ModelViewer()
{

}

void ModelViewer::draw()
{
   static ILightPtr sun = makeSunLight();
   
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   const GLfloat wf = static_cast<GLfloat>(w());
   const GLfloat hf = static_cast<GLfloat>(h());
   gluPerspective(45.0f, wf/hf, 0.1f, 50.0f);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glEnable(GL_LIGHTING);
   glEnable(GL_DEPTH_TEST);
   
   sun->apply();
   
   if (myModel) {
      glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
      glRotatef(45.0f, 0.0f, 1.0f, 0.0f);
      glTranslatef(1.5f, -2.6f, -1.5f);
      glColor3f(1.0f, 1.0f, 1.0f);
      myModel->render();
   }

   GLenum error = glGetError();
   if (error != GL_NO_ERROR) {   
      throw runtime_error
         ("OpenGL error: " + lexical_cast<string>(gluErrorString(error)));
   }
}

int ModelViewer::handle(int anEvent)
{
   return Fl_Gl_Window::handle(anEvent);
}
