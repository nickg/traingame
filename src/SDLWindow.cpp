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

#include "IWindow.hpp"
#include "ILogger.hpp"
#include "Maths.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;
using namespace std::tr1;

// Concrete implementation of SDL window
class SDLWindow : public IWindow, public IGraphics,
                  public enable_shared_from_this<SDLWindow> {
public:
   SDLWindow();
   ~SDLWindow();

   // IWindow interface
   void run(IScreenPtr aScreen);
   void quit();

   // IGraphics interface
   void setAmbient(double r, double g, double b);
   void setDiffuse(double r, double g, double b);
   void moveLight(double x, double y, double z);
   bool cuboidInViewFrustum(double x, double y, double z,
                            double sizeX, double sizeY, double sizeZ);
   bool cubeInViewFrustum(double x, double y, double z, double size);
   bool pointInViewFrustum(double x, double y, double z);
   void setCamera(const Vector<double>& aPos);
private:
   void resizeGLScene();
   void initGL();
   void processInput();
   void drawGLScene();
   
   bool amRunning;
   int myWidth, myHeight;
   IScreenPtr myScreen;
   Frustum myViewFrustum;
};

// Create the game window
SDLWindow::SDLWindow()
   : amRunning(false)
{
   // Start SDL
   if (SDL_Init(SDL_INIT_VIDEO) < 0) {
      ostringstream ss;
      ss << "Unable to initialise SDL: " << SDL_GetError();
      throw runtime_error(ss.str());
   }
   atexit(SDL_Quit);

   // Set the video mode
   const int DEFAULT_WIDTH = 800;
   const int DEFAULT_HEIGHT = 600;

   myWidth = DEFAULT_WIDTH;
   myHeight = DEFAULT_HEIGHT;

   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   if (SDL_SetVideoMode(myWidth, myHeight, 0, SDL_OPENGL) == NULL) {
      ostringstream ss;
      ss << "Unable to create OpenGL screen: " << SDL_GetError();
      throw runtime_error(ss.str());
   }

   resizeGLScene();

   // Start OpenGL
   initGL();

   log() << "Created " << myWidth << "x" << myHeight << " window";
}

// Run the game until the user quits
void SDLWindow::run(IScreenPtr aScreen)
{
   myScreen = aScreen;
   
   amRunning = true;
   do {
      processInput();
      drawGLScene();
   } while (amRunning);
}

// Stop the game cleanly
void SDLWindow::quit()
{
   amRunning = false;
}

// Called to set the camera position
void SDLWindow::setCamera(const Vector<double>& aPos)
{
   glTranslated(aPos.x, aPos.y, aPos.z);
   myViewFrustum = getViewFrustum();
}

// Render the next screen
void SDLWindow::drawGLScene()
{
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   setCamera(Vector<double>());

   myScreen->display(shared_from_this());

   SDL_GL_SwapBuffers();
}

// Check for SDL input events
void SDLWindow::processInput()
{
   SDL_Event e;

   while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
         // End the game
         quit();
         log() << "Window closed";
         break;
         
      case SDL_KEYDOWN:
         myScreen->onKeyDown(e.key.keysym.sym);
         break;

      case SDL_KEYUP:
         myScreen->onKeyUp(e.key.keysym.sym);
         break;
      }
   }
}

// Set initial OpenGL options
void SDLWindow::initGL()
{
   glShadeModel(GL_SMOOTH);
   glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

   glClearDepth(1.0f);
   glEnable(GL_DEPTH_TEST);
   glDepthFunc(GL_LEQUAL);

   glEnable(GL_TEXTURE_2D);
 
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
   
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT1);
}

// Change the perspective when the window is resized
void SDLWindow::resizeGLScene()
{
   if (myHeight == 0)
      myHeight = 1;

   glViewport(0, 0, myWidth, myHeight);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   // Calculate The Aspect Ratio Of The Window
   gluPerspective(45.0f, (GLfloat)myWidth/(GLfloat)myHeight, 0.1f, 50.0f);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

// Destroy the game window
SDLWindow::~SDLWindow()
{
   
}

// Set the value of the ambient light
void SDLWindow::setAmbient(double r, double g, double b)
{
   const GLfloat LightAmbient[] = { r, g, b, 1.0f };
   glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
}

// Set the diffuse light
void SDLWindow::setDiffuse(double r, double g, double b)
{
   const GLfloat LightDiffuse[] = { r, g, b, 1.0f };
   glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
}

// Change the light position
void SDLWindow::moveLight(double x, double y, double z)
{
   const GLfloat LightPosition[]= { x, y, z, 1.0f };
   glLightfv(GL_LIGHT1, GL_POSITION,LightPosition);
}

// Intersect a cuboid with the current view frustum
bool SDLWindow::cuboidInViewFrustum(double x, double y, double z,
                                    double sizeX, double sizeY, double sizeZ)
{
   return myViewFrustum.cuboidInFrustum(x, y, z, sizeX, sizeY, sizeZ);
}

// Intersect a cube with the current view frustum
bool SDLWindow::cubeInViewFrustum(double x, double y, double z, double size)
{
   return myViewFrustum.cubeInFrustum(x, y, z, size);
}

// True if the point is contained within the view frustum
bool SDLWindow::pointInViewFrustum(double x, double y, double z)
{
   return myViewFrustum.pointInFrustum(x, y, z);
}

// Construct and initialise an OpenGL SDL window
IWindowPtr makeSDLWindow()
{
   return shared_ptr<IWindow>(new SDLWindow);
}
