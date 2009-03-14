#include "IWindow.hpp"
#include "ILogger.hpp"

#include <stdexcept>
#include <sstream>
#include <cstdlib>

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

using namespace std;
using namespace std::tr1;

// Concrete implementation of SDL window
class SDLWindow : public IWindow {
public:
   SDLWindow();
   ~SDLWindow();

   void run();
   void quit();
private:
   void resizeGLScene();
   void initGL();
   void processInput();
   void drawGLScene();
   
   bool amRunning;
   int myWidth, myHeight;
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
void SDLWindow::run()
{
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

// Render the next screen
void SDLWindow::drawGLScene()
{
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

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
   gluPerspective(45.0f, (GLfloat)myWidth/(GLfloat)myHeight, 0.1f, 100.0f);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

// Destroy the game window
SDLWindow::~SDLWindow()
{
   
}

// Construct and initialise an OpenGL SDL window
IWindowPtr makeSDLWindow()
{
   return shared_ptr<IWindow>(new SDLWindow);
}
