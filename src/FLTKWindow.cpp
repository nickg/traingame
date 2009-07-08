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
#include "OpenGLHelper.hpp"

#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Box.h>
#include <FL/gl.h>
#include <GL/glu.h>

// An OpenGL window for the editor
class FLTKWindow : public IWindow, public IGraphics,                   
                   public Fl_Gl_Window, public IPickBuffer,
                   public enable_shared_from_this<FLTKWindow> {
public:
   FLTKWindow(int x, int y, int w, int h);
   ~FLTKWindow();

   // IWindow interface
   void run(IScreenPtr aScreen);
   void switchScreen(IScreenPtr aScreen);
   void quit();
   void takeScreenShot();
   int width() const;
   int height() const;
   void redrawHint();

   // IGraphics interface
   bool cuboidInViewFrustum(float x, float y, float z,
                            float sizeX, float sizeY, float sizeZ);
   bool cubeInViewFrustum(float x, float y, float z, float size);
   bool pointInViewFrustum(float x, float y, float z);
   void setCamera(const Vector<float>& aPos,
                  const Vector<float>& aRotation);
   void lookAt(const Vector<float> anEyePoint,
               const Vector<float> aTargetPoint);

   // Fl_Gl_Window interface
   void draw();
   int handle(int anEvent);

   // IPickBuffer inteface
   IGraphicsPtr beginPick(int x, int y);
   unsigned endPick();
private:
   void checkValid();

   IScreenPtr myScreen;
   Frustum myViewFrustum;

   // Picking data
   static const int SELECT_BUFFER_SZ = 128;
   GLuint mySelectBuffer[SELECT_BUFFER_SZ];
};

FLTKWindow::FLTKWindow(int x, int y, int w, int h)
   : Fl_Gl_Window(x, y, w, h)
{

}

FLTKWindow::~FLTKWindow()
{
   
}

void FLTKWindow::redrawHint()
{
   redraw();
}

void FLTKWindow::checkValid()
{
   if (!valid()) {
      initGL();
      resizeGLScene(shared_from_this());

      valid(1);
   }
}

void FLTKWindow::draw()
{
   checkValid();

   drawGLScene(shared_from_this(), shared_from_this(), myScreen);
}

int FLTKWindow::handle(int anEvent)
{
   static int lastX = 0, lastY = 0;

   int dx = 0, dy = 0;
   if (anEvent == FL_PUSH || FL_DRAG || FL_RELEASE || FL_MOVE) {
      dx = Fl::event_x() - lastX;
      dy = Fl::event_y() - lastY;
      lastX = Fl::event_x();
      lastY = Fl::event_y();
   }   
   
   MouseButton btn;
   if (Fl::event_button1())
      btn = MOUSE_LEFT;
   else if (Fl::event_button2())
      btn = MOUSE_MIDDLE;
   else if (Fl::event_button3())
      btn = MOUSE_RIGHT;
   else
      btn = MOUSE_UNKNOWN;
   
   // Do not call any OpenGL drawing functions in here as the context
   // won't be set up correctly
   switch (anEvent) {
   case FL_PUSH:
      // Mouse down event
      // Position in Fl::event_x() and Fl::event_y()
      myScreen->onMouseClick(shared_from_this(), Fl::event_x(),
                             Fl::event_y(), btn);
      return 1;
   case FL_DRAG:
      // Mouse moved while pressed down
      myScreen->onMouseMove(shared_from_this(), lastX, lastY, dx, dy);
      return 1;
   case FL_RELEASE:
      // Mouse up event
      myScreen->onMouseRelease(shared_from_this(), Fl::event_x(),
                               Fl::event_y(), btn);
      return 1;
   case FL_FOCUS:
   case FL_UNFOCUS:
      // Return 1 if we want keyboard events
      return 1;
   case FL_KEYBOARD:
      // Key in Fl::event_key() and ASCII in Fl::event_text()
      // Return 1 if we use this event
      return 1;
   case FL_SHORTCUT:
      // Shortcut key pressed
      return 1;
   default:
      // Do not handle this event
      return Fl_Gl_Window::handle(anEvent);
   }
}

void FLTKWindow::run(IScreenPtr aScreen)
{   
   switchScreen(aScreen);

   show();

   Fl::run();
}

void FLTKWindow::switchScreen(IScreenPtr aScreen)
{
   myScreen = aScreen;
}

void FLTKWindow::quit()
{
   error() << "FLTKWindow::quit not implemented";
}

void FLTKWindow::takeScreenShot()
{
   error() << "FLTKWindow::takeScreenShot not implemented";
}

int FLTKWindow::width() const
{
   return Fl_Gl_Window::w();
}

int FLTKWindow::height() const
{
   return Fl_Gl_Window::h();
}

// Set up OpenGL to pick out objects
IGraphicsPtr FLTKWindow::beginPick(int x, int y)
{
   ::beginPick(shared_from_this(), mySelectBuffer, x, y);
   return shared_from_this();
}

// Finish picking and return the name of the clicked object or zero
// It's *very* important that this is called exactly once for every
// beginPick or things will get very messed up
unsigned FLTKWindow::endPick()
{
   return ::endPick(mySelectBuffer);
}

// Called to set the camera position
void FLTKWindow::setCamera(const Vector<float>& aPos,
                          const Vector<float>& aRotation)
{
   glRotatef(aRotation.x, 1.0f, 0.0f, 0.0f);
   glRotatef(aRotation.y, 0.0f, 1.0f, 0.0f);
   glRotatef(aRotation.z, 0.0f, 0.0f, 1.0f);
   glTranslatef(aPos.x, aPos.y, aPos.z);

   myViewFrustum = getViewFrustum();
}

// A wrapper around gluLookAt
void FLTKWindow::lookAt(const Vector<float> anEyePoint,
                       const Vector<float> aTargetPoint)
{
   gluLookAt(anEyePoint.x, anEyePoint.y, anEyePoint.z,
             aTargetPoint.x, aTargetPoint.y, aTargetPoint.z,
             0, 1, 0);

   myViewFrustum = getViewFrustum();
}

// Intersect a cuboid with the current view frustum
bool FLTKWindow::cuboidInViewFrustum(float x, float y, float z,
                                    float sizeX, float sizeY, float sizeZ)
{
   return myViewFrustum.cuboidInFrustum(x, y, z, sizeX, sizeY, sizeZ);
}

// Intersect a cube with the current view frustum
bool FLTKWindow::cubeInViewFrustum(float x, float y, float z, float size)
{
   return myViewFrustum.cubeInFrustum(x, y, z, size);
}

// True if the point is contained within the view frustum
bool FLTKWindow::pointInViewFrustum(float x, float y, float z)
{
   return myViewFrustum.pointInFrustum(x, y, z);
}

// The main application window that contains the actual
// FLTKWindow and is hidden from the rest of the application
class FLTKAppWindow : public Fl_Window {
public:
   FLTKAppWindow(const string& aTitle, function<void ()> addControls);

   FLTKWindow* glWindow() const { return myGLWindow; }
private:
   FLTKWindow* myGLWindow;
   Fl_Window* myContainer;
};

FLTKAppWindow::FLTKAppWindow(const string& aTitle,
                             function<void ()> addControls)
   : Fl_Window(980, 600, aTitle.c_str())
{
   size_range(300, 240);
   resizable(this);

   const int panelW = 180;
   myGLWindow = new FLTKWindow(0, 0, w()-panelW, h());
   myContainer = new Fl_Window(w()-panelW, 0, panelW, h()); 
   end();

   myContainer->begin();
   addControls();
   myContainer->end();
   
   // Bit of a hack to get into a state where we can use OpenGL
   show();
   Fl::wait();

   myGLWindow->make_current();

   printGLVersion();
}

IWindowPtr makeFLTKWindow(const string& aTitle,
                          function<void ()> addControls)
{
   FLTKAppWindow* appWindow = new FLTKAppWindow(aTitle, addControls);
   
   return IWindowPtr(appWindow->glWindow());
}
