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
#include "IPickBuffer.hpp"
#include "Maths.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include <boost/lexical_cast.hpp>
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;
using namespace std::tr1;
using namespace boost;

// Concrete implementation of SDL window
class SDLWindow : public IWindow, public IGraphics, public IPickBuffer,
                  public enable_shared_from_this<SDLWindow> {
public:
   SDLWindow();
   ~SDLWindow();

   // IWindow interface
   void run(IScreenPtr aScreen);
   void switchScreen(IScreenPtr aScreen);
   void quit();

   // IGraphics interface
   bool cuboidInViewFrustum(double x, double y, double z,
                            double sizeX, double sizeY, double sizeZ);
   bool cubeInViewFrustum(double x, double y, double z, double size);
   bool pointInViewFrustum(double x, double y, double z);
   void setCamera(const Vector<double>& aPos,
                  const Vector<double>& aRotation);
   IGraphicsPtr beginPick(int x, int y);
   unsigned endPick();
private:
   void resizeGLScene();
   void initGL();
   void initCEGUI();
   void processInput();
   void drawGLScene();
   MouseButton fromSDLButton(Uint8 aSDLButton) const;
   
   bool amRunning;
   int myWidth, myHeight;
   IScreenPtr myScreen;
   Frustum myViewFrustum;
   bool willSkipNextFrame;

   // Picking data
   static const int SELECT_BUFFER_SZ = 128;
   GLuint mySelectBuffer[SELECT_BUFFER_SZ];

   static const float NEAR_CLIP, FAR_CLIP;
};

const float SDLWindow::NEAR_CLIP(0.1f);
const float SDLWindow::FAR_CLIP(50.0f);

// Calculation and display of the FPS rate
namespace {
   int theFrameCounter = 0;

   Uint32 updateFPS(Uint32 anInterval, void* unused)
   {
      const string title =
         "Trains! @ " + lexical_cast<string>(theFrameCounter) + " FPS";
      SDL_WM_SetCaption(title.c_str(), title.c_str());

      theFrameCounter = 0;
      
      return anInterval;
   }

   void frameComplete()
   {
      theFrameCounter++;
   }
}

// Create the game window
SDLWindow::SDLWindow()
   : amRunning(false), willSkipNextFrame(false)
{
   // Start SDL
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
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

   // Hide the window manager cursor
   //SDL_ShowCursor(SDL_DISABLE);
   
   resizeGLScene();

   // Start OpenGL
   initGL();

   log() << "Created " << myWidth << "x" << myHeight << " window";
}

// Destroy the game window
SDLWindow::~SDLWindow()
{
   
}

// Change the active screen while the game is running
void SDLWindow::switchScreen(IScreenPtr aScreen)
{
   assert(amRunning);

   myScreen = aScreen;
   willSkipNextFrame = true;
}

// Run the game until the user quits
void SDLWindow::run(IScreenPtr aScreen)
{
   assert(!amRunning);
   
   myScreen = aScreen;

   const unsigned targetFramerate = 30;
   const unsigned window = 1000 / targetFramerate;

   SDL_TimerID fpsTimer = SDL_AddTimer(1000, updateFPS, NULL);

   amRunning = true;
   do {
      unsigned tickStart = SDL_GetTicks();

      processInput();
      myScreen->update(shared_from_this());
      
      if (!willSkipNextFrame)
         drawGLScene();
      else
         willSkipNextFrame = false;

      // Limit the frame rate to `targetFramerate`
      unsigned tickNow = SDL_GetTicks();
      if (tickNow > tickStart + window) {
         // Missed the timing window
         // Try to catch up by skipping the next frame
         willSkipNextFrame = true;
      }
      else {
         // Got some time spare
         while (tickNow < tickStart + window)	{
            SDL_Delay(tickStart + window - tickNow);
            tickNow = SDL_GetTicks();
         }
      }

      frameComplete();
   } while (amRunning);

   SDL_RemoveTimer(fpsTimer);

   myScreen.reset();
}

// Stop the game cleanly
void SDLWindow::quit()
{
   amRunning = false;
}

// Called to set the camera position
void SDLWindow::setCamera(const Vector<double>& aPos,
                          const Vector<double>& aRotation)
{
   glRotated(aRotation.x, 1.0, 0.0, 0.0);
   glRotated(aRotation.y, 0.0, 1.0, 0.0);
   glRotated(aRotation.z, 0.0, 0.0, 1.0);
   glTranslated(aPos.x, aPos.y, aPos.z);
   myViewFrustum = getViewFrustum();
}

// Render the next screen
void SDLWindow::drawGLScene()
{
   // Set up for 3D mode
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();

   gluPerspective(45.0f, (GLfloat)myWidth/(GLfloat)myHeight,
                  NEAR_CLIP, FAR_CLIP);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Set default state
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_CULL_FACE);
  
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT1);
   
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   setCamera(Vector<double>(), Vector<double>());

   // Draw the 3D part
   myScreen->display(shared_from_this());

   // Set up for 2D
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0.0f, (GLfloat)myWidth, (GLfloat)myHeight, 0.0f);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   // Set 2D defaults
   glDisable(GL_LIGHTING);
   glDisable(GL_DEPTH_TEST);
   glEnable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
   glDisable(GL_CULL_FACE);

   SDL_GL_SwapBuffers();
}

// Convert an SDL button constant to a MouseButton
MouseButton SDLWindow::fromSDLButton(Uint8 aSDLButton) const
{
   switch (aSDLButton) {
   case SDL_BUTTON_LEFT: return MOUSE_LEFT;
   case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
   case SDL_BUTTON_RIGHT: return MOUSE_RIGHT;
   default:
      abort();
   }
}

// Check for SDL input events
void SDLWindow::processInput()
{
   SDL_Event e;

   // Send only one mouse motion event per frame
   bool haveSentMouseMotion = false;

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

      case SDL_MOUSEMOTION:
         if (!haveSentMouseMotion) {
            myScreen->onMouseMove(shared_from_this(),
                                  e.motion.x, e.motion.y);
            haveSentMouseMotion = true;
         }
         break;
         
      case SDL_MOUSEBUTTONDOWN:
         myScreen->onMouseClick(shared_from_this(),
                                e.button.x, e.button.y,
                                fromSDLButton(e.button.button));
         break;

      case SDL_MOUSEBUTTONUP:
         myScreen->onMouseRelease(shared_from_this(),
                                  e.button.x, e.button.y,
                                  fromSDLButton(e.button.button));
         break;
      }
   }
}

// Set initial OpenGL options
void SDLWindow::initGL()
{
   glShadeModel(GL_SMOOTH);
   glClearDepth(1.0f);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
   glDepthFunc(GL_LEQUAL);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
}

// Change the perspective when the window is resized
void SDLWindow::resizeGLScene()
{
   if (myHeight == 0)
      myHeight = 1;

   glViewport(0, 0, myWidth, myHeight);
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

// Set up OpenGL to pick out objects
IGraphicsPtr SDLWindow::beginPick(int x, int y)
{
   // Set up selection buffer
   glSelectBuffer(128, mySelectBuffer);

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
   gluPerspective(45.0f, (GLfloat)myWidth/(GLfloat)myHeight,
                  NEAR_CLIP, FAR_CLIP);

   glMatrixMode(GL_MODELVIEW);
   glInitNames();

   // Let the user render their stuff
   glLoadIdentity();
   return shared_from_this();
}

// Finish picking and return the name of the clicked object or zero
// It's *very* important that this is called exactly once for every
// beginPick or things will get very messed up
unsigned SDLWindow::endPick()
{
   int objectsFound = glRenderMode(GL_RENDER);
   
   // Go back to normal
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glMatrixMode(GL_MODELVIEW);
   
   // See if we found any objects
   if (objectsFound > 0) {
      // Find the object with the lowest depth
      unsigned int lowestDepth = mySelectBuffer[1];
      int selectedObject = mySelectBuffer[3];
      int i;
      
      // Go through all the objects found
      for (i = 1; i < objectsFound; i++) {
         // See if it's closer than the current nearest
         if (mySelectBuffer[(i*4) + 1] < lowestDepth)	{ // 4 values for each object
            lowestDepth = mySelectBuffer[(i * 4) + 1];
            selectedObject = mySelectBuffer[(i * 4) + 3];
         }
      }
      
      // Return closest object
      return selectedObject;
   }
   else
      return 0;
}

// Construct and initialise an OpenGL SDL window
IWindowPtr makeSDLWindow()
{
   return shared_ptr<IWindow>(new SDLWindow);
}
