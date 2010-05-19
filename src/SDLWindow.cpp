//
//  Copyright (C) 2009-2010  Nick Gasson
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
#include "OpenGLHelper.hpp"
#include "IConfig.hpp"
#include "IMesh.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <cassert>

#include <boost/lexical_cast.hpp>
#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

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
   void takeScreenShot();
   int width() const { return width_; }
   int height() const { return height_; }
   void redrawHint() {}

   // IGraphics interface
   bool cuboidInViewFrustum(float x, float y, float z,
                            float sizeX, float sizeY, float sizeZ);
   bool cubeInViewFrustum(float x, float y, float z, float size);
   bool pointInViewFrustum(float x, float y, float z);
   void setCamera(const Vector<float>& aPos,
                  const Vector<float>& aRotation);
   void lookAt(const Vector<float> anEyePoint,
               const Vector<float> aTargetPoint);

   // IPickBuffer interface
   IGraphicsPtr beginPick(int x, int y);
   unsigned endPick();
private:
   void processInput();
   MouseButton fromSDLButton(Uint8 aSDLButton) const;
   void captureFrame() const;
   
   bool amRunning;
   int width_, height_;
   IScreenPtr screen;
   bool willSkipNextFrame;
   bool willTakeScreenShot;
   Frustum viewFrustum;

   // Picking data
   static const int SELECT_BUFFER_SZ = 128;
   GLuint mySelectBuffer[SELECT_BUFFER_SZ];
};

// Calculation and display of the FPS rate
namespace {
   int theFrameCounter = 0;
   int theLastFPS = 0;

   Uint32 updateFPS(Uint32 anInterval, void* thread);

   // A wrapper around SDL times
   struct FrameTimerThread {
      FrameTimerThread()
      {
         myTimer = SDL_AddTimer(1000, updateFPS, this);
      }

      ~FrameTimerThread()
      {
         // Finalise properly when an exception is thrown
         SDL_RemoveTimer(myTimer);
      }

      // Should be called from the main thread
      void updateTitle()
      {
         if (shouldUpdateTitle) {
            int avgTriangles = getAverageTriangleCount();
            
            const string title =
               "Trains! @ " + lexical_cast<string>(theLastFPS)
               + " FPS [" + lexical_cast<string>(avgTriangles)
               + " triangles]";
            SDL_WM_SetCaption(title.c_str(), title.c_str());
         }
      }

      SDL_TimerID myTimer;
      bool shouldUpdateTitle;
   };

   Uint32 updateFPS(Uint32 anInterval, void* thread)
   {
      theLastFPS = theFrameCounter;
      theFrameCounter = 0;

      static_cast<FrameTimerThread*>(thread)->shouldUpdateTitle = true;
      
      return anInterval;
   }

   void frameComplete()
   {
      theFrameCounter++;

      updateRenderStats();
   }
}

// Create the game window
SDLWindow::SDLWindow()
   : amRunning(false), willSkipNextFrame(false),
     willTakeScreenShot(false)
{
   IConfigPtr cfg = getConfig();
      
   // Start SDL
   if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
      ostringstream ss;
      ss << "Unable to initialise SDL: " << SDL_GetError();
      throw runtime_error(ss.str());
   }
   atexit(SDL_Quit);

   // Set the video mode
   cfg->get("XRes", width_);
   cfg->get("YRes", height_);

   SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
   if (SDL_SetVideoMode(width_, height_, 0, SDL_OPENGL) == NULL) {
      ostringstream ss;
      ss << "Unable to create OpenGL screen: " << SDL_GetError();
      throw runtime_error(ss.str());
   }

   // Turn on key repeat
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

   // Hide the window manager cursor
   //SDL_ShowCursor(SDL_DISABLE);
   
   // Start OpenGL
   printGLVersion();
   initGL();

   log() << "Created " << width_ << "x" << height_ << " window";
}

// Destroy the game window
SDLWindow::~SDLWindow()
{
   
}

// Make a screen capture at the end of this frame
void SDLWindow::takeScreenShot()
{
   willTakeScreenShot = true;
}

// Change the active screen while the game is running
void SDLWindow::switchScreen(IScreenPtr aScreen)
{
   assert(amRunning);

   screen = aScreen;
   willSkipNextFrame = true;
}

// Run the game until the user quits
void SDLWindow::run(IScreenPtr aScreen)
{
   assert(!amRunning);
   
   screen = aScreen;

   FrameTimerThread fpsTimer;

   unsigned lastTick = SDL_GetTicks();

   // Wait a few milliseconds to get a reasonable tick delta
   SDL_Delay(1);
   
   amRunning = true;
   do {
      unsigned tickStart = SDL_GetTicks();
      int delta = static_cast<int>(tickStart - lastTick);

      try {
         processInput();
         screen->update(shared_from_this(), delta);
         
         if (!willSkipNextFrame) {
            drawGLScene(shared_from_this(), shared_from_this(), screen);
            SDL_GL_SwapBuffers();
         }
         else
            willSkipNextFrame = false;
      }
      catch (runtime_error& e) {
         error() << "Caught exception: " << e.what();
         amRunning = false;
      }

      if (willTakeScreenShot) {
         captureFrame();
         willTakeScreenShot = false;
      }

      // Release the CPU for a little while
      SDL_Delay(1);

      frameComplete();
      fpsTimer.updateTitle();
      lastTick = tickStart;
   } while (amRunning);
   
   screen.reset();
}

// Stop the game cleanly
void SDLWindow::quit()
{
   amRunning = false;
}

// Convert an SDL button constant to a MouseButton
MouseButton SDLWindow::fromSDLButton(Uint8 aSDLButton) const
{
   switch (aSDLButton) {
   case SDL_BUTTON_LEFT: return MOUSE_LEFT;
   case SDL_BUTTON_MIDDLE: return MOUSE_MIDDLE;
   case SDL_BUTTON_RIGHT: return MOUSE_RIGHT;
   case SDL_BUTTON_WHEELUP: return MOUSE_WHEEL_UP;
   case SDL_BUTTON_WHEELDOWN: return MOUSE_WHEEL_DOWN;
   default:
      return MOUSE_UNKNOWN;
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
         screen->onKeyDown(e.key.keysym.sym);
         break;

      case SDL_KEYUP:
         screen->onKeyUp(e.key.keysym.sym);
         break;

      case SDL_MOUSEMOTION:
         if (!haveSentMouseMotion) {
            screen->onMouseMove(shared_from_this(),
                                  e.motion.x, e.motion.y,
                                  e.motion.xrel, e.motion.yrel);
            haveSentMouseMotion = true;
         }
         break;
         
      case SDL_MOUSEBUTTONDOWN:
         screen->onMouseClick(shared_from_this(),
                                e.button.x, e.button.y,
                                fromSDLButton(e.button.button));
         break;

      case SDL_MOUSEBUTTONUP:
         screen->onMouseRelease(shared_from_this(),
                                  e.button.x, e.button.y,
                                  fromSDLButton(e.button.button));
         break;

      case SDL_VIDEORESIZE:
         width_ = e.resize.w;
         height_ = e.resize.h;
         
         resizeGLScene(shared_from_this());
         break;
      }
   }
}

// Set up OpenGL to pick out objects
IGraphicsPtr SDLWindow::beginPick(int x, int y)
{
   ::beginPick(shared_from_this(), mySelectBuffer, x, y);
   return shared_from_this();
}

// Finish picking and return the name of the clicked object or zero
// It's *very* important that this is called exactly once for every
// beginPick or things will get very messed up
unsigned SDLWindow::endPick()
{
   return ::endPick(mySelectBuffer);
}

// Called to set the camera position
void SDLWindow::setCamera(const Vector<float>& aPos,
                          const Vector<float>& aRotation)
{
   glRotatef(aRotation.x, 1.0f, 0.0f, 0.0f);
   glRotatef(aRotation.y, 0.0f, 1.0f, 0.0f);
   glRotatef(aRotation.z, 0.0f, 0.0f, 1.0f);
   glTranslatef(aPos.x, aPos.y, aPos.z);

   viewFrustum = getViewFrustum();
}

// A wrapper around gluLookAt
void SDLWindow::lookAt(const Vector<float> anEyePoint,
                       const Vector<float> aTargetPoint)
{
   gluLookAt(anEyePoint.x, anEyePoint.y, anEyePoint.z,
             aTargetPoint.x, aTargetPoint.y, aTargetPoint.z,
             0, 1, 0);

   viewFrustum = getViewFrustum();
}

// Intersect a cuboid with the current view frustum
bool SDLWindow::cuboidInViewFrustum(float x, float y, float z,
                                    float sizeX, float sizeY, float sizeZ)
{
   return viewFrustum.cuboidInFrustum(x, y, z, sizeX, sizeY, sizeZ);
}

// Intersect a cube with the current view frustum
bool SDLWindow::cubeInViewFrustum(float x, float y, float z, float size)
{
   return viewFrustum.cubeInFrustum(x, y, z, size);
}

// True if the point is contained within the view frustum
bool SDLWindow::pointInViewFrustum(float x, float y, float z)
{
   return viewFrustum.pointInFrustum(x, y, z);
}

// Capture the OpenGL pixels and save them to a file
void SDLWindow::captureFrame() const
{
   static int fileNumber = 1;
   
   const string fileName
      ("screenshot" + lexical_cast<string>(fileNumber++) + ".bmp");

   SDL_Surface* temp = SDL_CreateRGBSurface
      (SDL_SWSURFACE, width_, height_, 24,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
       0x000000FF, 0x0000FF00, 0x00FF0000, 0
#else
       0x00FF0000, 0x0000FF00, 0x000000FF, 0
#endif
       );
   assert(temp);

   const int w = width_;
   const int h = height_;
   unsigned char* pixels = new unsigned char[3 * w * h];

   glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, pixels);

   for (int i = 0; i < h; i++)
      memcpy(((char*)temp->pixels) + temp->pitch * i, pixels + 3*w * (h-i-1), w*3);
   delete[] pixels;

   SDL_SaveBMP(temp, fileName.c_str());
   SDL_FreeSurface(temp);

   log() << "Wrote screen shot to " << fileName;
}

// Construct and initialise an OpenGL SDL window
IWindowPtr makeSDLWindow()
{
   return std::tr1::shared_ptr<IWindow>(new SDLWindow);
}
