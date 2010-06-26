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

#include <GL/glew.h>  // Must be included before other GL headers

#include "OpenGLHelper.hpp"
#include "ILogger.hpp"
#include "IConfig.hpp"

#include <stdexcept>

#include <GL/gl.h>
#include <GL/glu.h>

#include <boost/lexical_cast.hpp>

void checkGLError()
{
   using namespace boost;
   
   GLenum error = glGetError();
   if (error != GL_NO_ERROR) {   
      throw runtime_error
         ("OpenGL error: " + lexical_cast<string>(gluErrorString(error)));
   }
}

void drawGLScene(IWindowPtr a_window, IGraphicsPtr a_context, IScreenPtr a_screen)
{
   using namespace boost;
   
   // Set up for 3D mode
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   const int w = a_window->width();
   const int h = a_window->height();

   IConfigPtr cfg = get_config();
   gluPerspective(45.0f, (GLfloat)w/(GLfloat)h,
      cfg->get<float>("NearClip"),
      cfg->get<float>("FarClip"));

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Set default state
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_CULL_FACE);
  
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   // Draw the 3D part
   a_screen->display(a_context);

   // Set up for 2D
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0.0f, (GLfloat)w, (GLfloat)h, 0.0f);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Set 2D defaults
   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_CULL_FACE);

   // Draw the 2D part
   a_screen->overlay();

   // Check for OpenGL errors
   checkGLError();
}

// Report the current OpenGL version
void printGLVersion()
{
   log() << "OpenGL version: " << glGetString(GL_VERSION);
   log() << "GLEW version: " << glewGetString(GLEW_VERSION);
}

// Set initial OpenGL options
void initGL()
{
   using namespace boost;
   
   glShadeModel(GL_SMOOTH);
   glClearDepth(1.0f);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
   glDepthFunc(GL_LEQUAL);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   // Clear to the sky colour
   //glClear_color(0.6f, 0.7f, 0.8f, 1.0f);
   glClearColor(176.0f/255.0f, 196.0f/255.0f, 222.0f/255.0f, 1.0f);
   
   // Check for OpenGL extensions   
   GLenum err = glewInit();
   if (err != GLEW_OK)
      throw runtime_error("GLEW initialisation failed: "
         + lexical_cast<string>(glewGetErrorString(err)));

}

// Set the current viewport
void resizeGLScene(IWindowPtr a_window)
{
   glViewport(0, 0, a_window->width(), a_window->height());
}

void begin_pick(IWindowPtr a_window, unsigned* a_buffer, int x, int y)
{
   // Set up selection buffer
   glSelectBuffer(128, a_buffer);

   // Get viewport coordinates
   GLint viewport_coords[4];
   glGetIntegerv(GL_VIEWPORT, viewport_coords);	
   
   // Switch to projection matrix
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   
   // Render the objects, but don't change the frame buffer
   glRenderMode(GL_SELECT);
   glLoadIdentity();	
   
   // Set picking matrix
   gluPickMatrix(x, viewport_coords[3] - y, 2, 2, viewport_coords);

   // Just set the perspective
   IConfigPtr cfg = get_config();
   gluPerspective(45.0f, (GLfloat)(a_window->width())/(GLfloat)(a_window->height()),
      cfg->get<float>("NearClip"),
      cfg->get<float>("FarClip"));

   glMatrixMode(GL_MODELVIEW);
   glInitNames();

   // Let the user render their stuff
   glLoadIdentity();
}

unsigned end_pick(unsigned* a_buffer)
{   
   int objects_found = glRenderMode(GL_RENDER);
   
   // Go back to normal
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   
   // See if we found any objects
   if (objects_found > 0) {
      // Find the object with the lowest depth
      unsigned int lowest_depth = a_buffer[1];
      int selected_object = a_buffer[3];
      
      // Go through all the objects found
      for (int i = 1; i < objects_found; i++) {
         // See if it's closer than the current nearest
         if (a_buffer[(i*4) + 1] < lowest_depth) { // 4 values for each object
            lowest_depth = a_buffer[(i * 4) + 1];
            selected_object = a_buffer[(i * 4) + 3];
         }
      }
      
      // Return closest object
      return selected_object;
   }
   else
      return 0;
}
