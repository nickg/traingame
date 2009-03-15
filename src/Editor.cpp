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

#include "Editor.hpp"
#include "ILogger.hpp"
#include "IModel.hpp"
#include "IMap.hpp"
#include "Maths.hpp"

#include <GL/gl.h>

using namespace std;

// Concrete editor class
class Editor : public IScreen {
public:
   Editor();
   
   void display(IGraphicsPtr aContext);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);

private:
   IModelPtr m;
   IMapPtr myMap;

   Vector<double> myPosition;
   Vector<double> myMovement;
};

Editor::Editor()
   : myPosition(2.0, -5.0, -5.0)
{
   const string fileName("/home/nick/stompstomp.obj");
   m = loadModel(fileName);

   myMap = makeEmptyMap(128, 128);
}

// Render the next frame
void Editor::display(IGraphicsPtr aContext)
{
   myPosition += myMovement;
   glRotated(45.0, 1.0, 0.0, 0.0);
   glRotated(45.0, 0.0, 1.0, 0.0);
   aContext->setCamera(myPosition);
   
   aContext->setAmbient(0.5, 0.5, 0.5);
   aContext->setDiffuse(1.0, 1.0, 1.0);
   aContext->moveLight(0.0, 20.0, 0.0);
   
   /*glTranslatef(0.0f, 0.0f, -15.0f);
   glBegin(GL_TRIANGLES);
   glColor3f(1.0f, 0.0f, 0.0f);
   glVertex3f(1.0f, -1.0f, 0.0f);
   glColor3f(0.0f, 1.0f, 0.0f);
   glVertex3f(1.0f, 0.0f, 0.0f);
   glColor3f(0.0f, 0.0f, 1.0f);
   glVertex3f(0.0f, 0.0f, 0.0f);
   glEnd();

   glTranslatef(-5.0f, 0.0f, 0.0f);
   m->render();
   */
   //   glTranslated(0.0, -4.0, -12.0);
   myMap->render(aContext);
}

void Editor::onKeyUp(SDLKey aKey)
{
   switch (aKey) {
   case SDLK_w:
   case SDLK_s:
      myMovement.z = 0.0;
      break;
   case SDLK_a:
   case SDLK_d:
      myMovement.x = 0.0;
      break;
   default:
      break;
   }
}

void Editor::onKeyDown(SDLKey aKey)
{
   switch (aKey) {
   case SDLK_w:
      myMovement.z = 1.0;
      break;
   case SDLK_s:
      myMovement.z = -1.0;
      break;
   case SDLK_a:
      myMovement.x = 1.0;
      break;
   case SDLK_d:
      myMovement.x = -1.0;
      break;
   default:
      break;
   }
}

// Create an instance of the editor screen
IScreenPtr makeEditorScreen()
{
   return IScreenPtr(new Editor);
}
