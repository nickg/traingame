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

#include "OpenGLHelper.hpp"
#include "ILogger.hpp"

#include <stdexcept>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>

#include <boost/lexical_cast.hpp>

void drawGLScene(IWindowPtr aWindow, IGraphicsPtr aContext, IScreenPtr aScreen)
{
   using namespace boost;
   
   // Set up for 3D mode
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   const int w = aWindow->width();
   const int h = aWindow->height();

   gluPerspective(45.0f, (GLfloat)w/(GLfloat)h,
                  NEAR_CLIP, FAR_CLIP);

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
   aScreen->display(aContext);

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
   aScreen->overlay();

   // Check for OpenGL errors
   GLenum error = glGetError();
   if (error != GL_NO_ERROR) {   
      throw runtime_error
         ("OpenGL error: " + lexical_cast<string>(gluErrorString(error)));
   }
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

   // Check for OpenGL extensions
   log() << "OpenGL version: " << glGetString(GL_VERSION);
   log() << "GLEW version: " << glewGetString(GLEW_VERSION);

   GLenum err = glewInit();
   if (err != GLEW_OK)
      throw runtime_error("GLEW initialisation failed: "
                          + lexical_cast<string>(glewGetErrorString(err)));

}

// Set the current viewport
void resizeGLScene(IWindowPtr aWindow)
{
   glViewport(0, 0, aWindow->width(), aWindow->height());
}

void beginPick(IWindowPtr aWindow, unsigned* aBuffer, int x, int y)
{
   // Set up selection buffer
   glSelectBuffer(128, aBuffer);

   // Get viewport coordinates
   GLint viewportCoords[4];
   glGetIntegerv(GL_VIEWPORT, viewportCoords);	
   
   // Switch to projection matrix
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   
   // Render the objects, but don't change the frame buffer
   glRenderMode(GL_SELECT);
   glLoadIdentity();	
   
   // Set picking matrix
   gluPickMatrix(x, viewportCoords[3] - y, 2, 2, viewportCoords);

   // Just set the perspective
   gluPerspective(45.0f, (GLfloat)(aWindow->width())/(GLfloat)(aWindow->height()),
                  NEAR_CLIP, FAR_CLIP);

   glMatrixMode(GL_MODELVIEW);
   glInitNames();

   // Let the user render their stuff
   glLoadIdentity();
}

unsigned endPick(unsigned* aBuffer)
{   
   int objectsFound = glRenderMode(GL_RENDER);
   
   // Go back to normal
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   
   // See if we found any objects
   if (objectsFound > 0) {
      // Find the object with the lowest depth
      unsigned int lowestDepth = aBuffer[1];
      int selectedObject = aBuffer[3];
      
      // Go through all the objects found
      for (int i = 1; i < objectsFound; i++) {
         // See if it's closer than the current nearest
         if (aBuffer[(i*4) + 1] < lowestDepth)	{ // 4 values for each object
            lowestDepth = aBuffer[(i * 4) + 1];
            selectedObject = aBuffer[(i * 4) + 3];
         }
      }
      
      // Return closest object
      return selectedObject;
   }
   else
      return 0;
}

// Called to set the camera position
void OpenGLGraphics::setCamera(const Vector<float>& aPos,
                               const Vector<float>& aRotation)
{
   glRotatef(aRotation.x, 1.0f, 0.0f, 0.0f);
   glRotatef(aRotation.y, 0.0f, 1.0f, 0.0f);
   glRotatef(aRotation.z, 0.0f, 0.0f, 1.0f);
   glTranslatef(aPos.x, aPos.y, aPos.z);

   myViewFrustum = getViewFrustum();
}

OpenGLGraphics::OpenGLGraphics()
{

}

OpenGLGraphics::~OpenGLGraphics()
{

}

// A wrapper around gluLookAt
void OpenGLGraphics::lookAt(const Vector<float> anEyePoint,
                            const Vector<float> aTargetPoint)
{
   gluLookAt(anEyePoint.x, anEyePoint.y, anEyePoint.z,
             aTargetPoint.x, aTargetPoint.y, aTargetPoint.z,
             0, 1, 0);

   myViewFrustum = getViewFrustum();
}

// Intersect a cuboid with the current view frustum
bool OpenGLGraphics::cuboidInViewFrustum(float x, float y, float z,
                                         float sizeX, float sizeY, float sizeZ)
{
   return myViewFrustum.cuboidInFrustum(x, y, z, sizeX, sizeY, sizeZ);
}

// Intersect a cube with the current view frustum
bool OpenGLGraphics::cubeInViewFrustum(float x, float y, float z, float size)
{
   return myViewFrustum.cubeInFrustum(x, y, z, size);
}

// True if the point is contained within the view frustum
bool OpenGLGraphics::pointInViewFrustum(float x, float y, float z)
{
   return myViewFrustum.pointInFrustum(x, y, z);
}
