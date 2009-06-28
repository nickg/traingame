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
#include <FL/gl.h>

// An OpenGL window that supports FLTK widgets for the editor
class FLTKWindow : public IWindow, public OpenGLGraphics,                   
                   public Fl_Gl_Window, public IPickBuffer,
                   public enable_shared_from_this<FLTKWindow> {
public:
   FLTKWindow();
   ~FLTKWindow();

   // IWindow interface
   void run(IScreenPtr aScreen);
   void switchScreen(IScreenPtr aScreen);
   void quit();
   void takeScreenShot();
   int width() const;
   int height() const;

   // Fl_Gl_Window interface
   void draw();
   int handle(int anEvent);

   // IPickBuffer inteface
   IGraphicsPtr beginPick(int x, int y);
   unsigned endPick();
private:
   void checkValid();

   IScreenPtr myScreen;

   // Picking data
   static const int SELECT_BUFFER_SZ = 128;
   GLuint mySelectBuffer[SELECT_BUFFER_SZ];
};

FLTKWindow::FLTKWindow()
   : Fl_Gl_Window(640, 480)
{
   size_range(300, 240);
   resizable(this);
   //myWindow = new Fl_Window(300, 180);
   /*   myBox = new Fl_Box(20, 40, 260, 100, "Hello, World!");

   myBox->box(FL_UP_BOX);
   myBox->labelsize(36);
   myBox->labelfont(FL_BOLD + FL_ITALIC);
   myBox->labeltype(FL_SHADOW_LABEL);
   */
   //myWindow->end();

   // Bit of a hack to get into a state where we can use OpenGL
   show();
   Fl::wait();

   make_current();
}

FLTKWindow::~FLTKWindow()
{
   
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
   // Do not call any OpenGL drawing functions in here as the context
   // won't be set up correctly
   switch (anEvent) {
   case FL_PUSH:
      // Mouse down event
      // Position in Fl::event_x() and Fl::event_y()
      myScreen->onMouseClick(shared_from_this(), Fl::event_x(),
                             Fl::event_y(), MOUSE_LEFT);
      return 1;
   case FL_DRAG:
      // Mouse moved while pressed down
      myScreen->onMouseMove(shared_from_this(), Fl::event_x(),
                            Fl::event_y(), 1, 1);
      return 1;
   case FL_RELEASE:
      // Mouse up event
      myScreen->onMouseRelease(shared_from_this(), Fl::event_x(),
                               Fl::event_y(), MOUSE_LEFT);
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

IWindowPtr makeFLTKWindow()
{
   return IWindowPtr(new FLTKWindow);
}
