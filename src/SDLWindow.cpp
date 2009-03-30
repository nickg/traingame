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

#include <SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <unistd.h>
#include <CEGUI.h>
#include <CEGUIDefaultResourceProvider.h>

#include "SDLToCEGUIKey.hpp"

// Using the OpenGL implementation of CEGUI
#include <RendererModules/OpenGLGUIRenderer/openglrenderer.h>

// Using the Xerces parser for CEGUI
#include <XMLParserModules/XercesParser/CEGUIXercesParser.h>

using namespace std;
using namespace std::tr1;

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
   void setAmbient(double r, double g, double b);
   void setDiffuse(double r, double g, double b);
   void moveLight(double x, double y, double z);
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
   int sdlButtonToInt(Uint8 aSDLButton) const;
   
   bool amRunning;
   int myWidth, myHeight;
   IScreenPtr myScreen;
   Frustum myViewFrustum;
   bool willSkipNextFrame;

   // Picking data
   static const int SELECT_BUFFER_SZ = 128;
   GLuint mySelectBuffer[SELECT_BUFFER_SZ];

   // CEGUI objects
   CEGUI::OpenGLRenderer* myRenderer;   

   static const float NEAR_CLIP, FAR_CLIP;
};

const float SDLWindow::NEAR_CLIP(0.1f);
const float SDLWindow::FAR_CLIP(50.0f);

// Create the game window
SDLWindow::SDLWindow()
   : amRunning(false), willSkipNextFrame(false), myRenderer(NULL)
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

   // Set SDL options
   SDL_ShowCursor(SDL_DISABLE);
   SDL_EnableUNICODE(1);
   SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY, SDL_DEFAULT_REPEAT_INTERVAL);

   resizeGLScene();

   // Start OpenGL
   initGL();

   log() << "Created " << myWidth << "x" << myHeight << " window";

   initCEGUI();
}

// Destroy the game window
SDLWindow::~SDLWindow()
{
   delete myRenderer;
}

// Start CEGUI and load resources
void SDLWindow::initCEGUI()
{
   using namespace CEGUI;

   // Starting CEGUI affects some OpenGL attribute bits
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   
   // Start CEGUI
   myRenderer = new CEGUI::OpenGLRenderer(0, myWidth, myHeight);
   new CEGUI::System(myRenderer);

   // Initialise the directories for the default resource provider
   CEGUI::DefaultResourceProvider* rp = static_cast<CEGUI::DefaultResourceProvider*>
      (CEGUI::System::getSingleton().getResourceProvider());

   rp->setResourceGroupDirectory("schemes", "data/gui/schemes/");
   rp->setResourceGroupDirectory("imagesets", "data/gui/imagesets/");
   rp->setResourceGroupDirectory("fonts", "data/gui/fonts/");
   rp->setResourceGroupDirectory("layouts", "data/gui/layouts/");
   rp->setResourceGroupDirectory("looknfeels", "data/gui/looknfeel/");
   rp->setResourceGroupDirectory("lua_scripts", "data/gui/lua_scripts/");
   rp->setResourceGroupDirectory("schemas", "data/gui/schemas/");
   
   // Set the default resource groups to be used
   CEGUI::Imageset::setDefaultResourceGroup("imagesets");
   CEGUI::Font::setDefaultResourceGroup("fonts");
   CEGUI::Scheme::setDefaultResourceGroup("schemes");
   CEGUI::WidgetLookManager::setDefaultResourceGroup("looknfeels");
   CEGUI::WindowManager::setDefaultResourceGroup("layouts");
   CEGUI::ScriptModule::setDefaultResourceGroup("lua_scripts");
   CEGUI::XercesParser::setSchemaDefaultResourceGroup("schemas");

   // Load the scheme file which autoloads the image set
   CEGUI::SchemeManager::getSingleton().loadScheme("Trains.scheme");
   CEGUI::SchemeManager::getSingleton().loadScheme("TaharezLook.scheme");

   // Defaults
   System::getSingleton().setDefaultFont("DejaVuSans-10");
   System::getSingleton().setDefaultMouseCursor("Trains", "MouseArrow");
   
   log() << "CEGUI initialised";

   // Restore original attribute bits
   glPopAttrib();
}

// Change the active screen while the game is running
void SDLWindow::switchScreen(IScreenPtr aScreen)
{
   assert(amRunning);

   // Clean up any windows the previous screen created
   CEGUI::WindowManager::getSingleton().destroyAllWindows();
   
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

   unsigned tickEnd = SDL_GetTicks();
   
   amRunning = true;
   do {
      unsigned tickStart = tickEnd;

      processInput();
      myScreen->update(shared_from_this());
      
      if (!willSkipNextFrame)
         drawGLScene();
      else
         willSkipNextFrame = false;

      // Limit the frame rate
      unsigned tickNow = SDL_GetTicks();
      if (tickNow > tickStart + window) {
         log() << "Missed window by " << tickNow - tickStart - window
               << "ms (skipping next frame)";
         willSkipNextFrame = true;
      }
      else {
         while (tickNow < tickStart + window)	{
            //log() << "spare ms: " << (tickStart + window - tickNow);
            usleep((tickStart + window - tickNow) * 1000);
            tickNow = SDL_GetTicks();
         }
      }

      // Update the CEGUI internal timer
      tickEnd = SDL_GetTicks();
      
      float t = 0.001f * (tickEnd - tickStart);
      CEGUI::System::getSingleton().injectTimePulse(t);
   } while (amRunning);

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
   glTranslated(aPos.x, aPos.y, aPos.z);
   myViewFrustum = getViewFrustum();
}

// Render the next screen
void SDLWindow::drawGLScene()
{   
   // Clear the screen
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glLoadIdentity();

   setCamera(Vector<double>(), Vector<double>());
   
   myScreen->display(shared_from_this());

   glPushAttrib(GL_ALL_ATTRIB_BITS);
   CEGUI::System::getSingleton().renderGUI();
   glPopAttrib();

   SDL_GL_SwapBuffers();
}

// Convert an SDL button constant to an integer or -1
int SDLWindow::sdlButtonToInt(Uint8 aSDLButton) const
{
   switch (aSDLButton) {
   case SDL_BUTTON_LEFT: return 0;
   case SDL_BUTTON_MIDDLE: return 1;
   case SDL_BUTTON_RIGHT: return 2;
   default: return -1;
   }
}

// Check for SDL input events
void SDLWindow::processInput()
{
   using namespace CEGUI;

   SDL_Event e;
   MouseButton mb;
   bool valid;
   CEGUI::uint kc;

   while (SDL_PollEvent(&e)) {
      switch (e.type) {
      case SDL_QUIT:
         // End the game
         quit();
         log() << "Window closed";
         break;
         
      case SDL_KEYDOWN:
         kc = SDLKeyToCEGUIKey(e.key.keysym.sym);
         if (!System::getSingleton().injectKeyDown(kc))
            myScreen->onKeyDown(e.key.keysym.sym);

         // Handle Unicode keys
         if (e.key.keysym.unicode != 0)
            System::getSingleton().injectChar(e.key.keysym.unicode);

         break;

      case SDL_KEYUP:
         kc = SDLKeyToCEGUIKey(e.key.keysym.sym);
         if (!System::getSingleton().injectKeyUp(kc))
            myScreen->onKeyUp(e.key.keysym.sym);
         
         break;

      case SDL_MOUSEMOTION:
         if (!CEGUI::System::getSingleton().injectMousePosition
             (static_cast<float>(e.motion.x),
              static_cast<float>(e.motion.y)))
            myScreen->onMouseMove(shared_from_this(),
                                  e.motion.x, e.motion.y);
         break;
         
      case SDL_MOUSEBUTTONDOWN:
         valid = true;
         switch (e.button.button) {
            // Handle real mouse buttons
         case SDL_BUTTON_LEFT:
            mb = LeftButton;
            break;
         case SDL_BUTTON_MIDDLE:
            mb = CEGUI::MiddleButton;
            break;
         case SDL_BUTTON_RIGHT:
            mb = CEGUI::RightButton;
            break;
		
            // Handle the mouse wheel
         case SDL_BUTTON_WHEELDOWN:
            CEGUI::System::getSingleton().injectMouseWheelChange(-1);
            valid = false;
            break;
         case SDL_BUTTON_WHEELUP:
            CEGUI::System::getSingleton().injectMouseWheelChange(+1);
            valid = false;
            break;

         default:
            valid = false;
         }

         if (valid && !System::getSingleton().injectMouseButtonDown(mb))
            myScreen->onMouseClick(shared_from_this(),
                                   e.button.x, e.button.y,
                                   sdlButtonToInt(e.button.button));
         break;

      case SDL_MOUSEBUTTONUP:
         valid = true;
         switch (e.button.button) {
         case SDL_BUTTON_LEFT:
            mb = CEGUI::LeftButton;
            break;
         case SDL_BUTTON_MIDDLE:
            mb = CEGUI::MiddleButton;
            break;
         case SDL_BUTTON_RIGHT:
            mb = CEGUI::RightButton;
            break;

         default:
            valid = false;
         }

         if (valid && !System::getSingleton().injectMouseButtonUp(mb))
            myScreen->onMouseRelease(shared_from_this(),
                                     e.button.x, e.button.y,
                                     sdlButtonToInt(e.button.button));
         
         break;

      case SDL_VIDEORESIZE:
         myRenderer->grabTextures();
         // Your resize code here, including the SDL_SetVideoMode call
         myRenderer->restoreTextures();
         myRenderer->setDisplaySize(CEGUI::Size(e.resize.w, e.resize.h));
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
   gluPerspective(45.0f, (GLfloat)myWidth/(GLfloat)myHeight,
                  NEAR_CLIP, FAR_CLIP);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
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
   const GLfloat LightPosition[]= { x, y, z, 0.0f };
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

   log() << objectsFound << " objects found after picking";
   
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
