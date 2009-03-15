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

#include "GameScreens.hpp"
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
   : myPosition(2.0, -8.0, -10.0)
{
   const string fileName("/home/nick/traingame/train.obj");
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
   aContext->setDiffuse(0.8, 0.8, 0.8);
   aContext->moveLight(0.0, 50.0, 0.0);
   
   glPushMatrix();
   glScaled(0.6, 0.6, 0.6);
   glTranslated(5.0, 0.5, 5.0);
   glRotated(-69.0, 0.0, 1.0, 0.0);
   m->render();
   glPopMatrix();

   myMap->render(aContext);
}

void Editor::onKeyUp(SDLKey aKey)
{
   switch (aKey) {
   case SDLK_w:
   case SDLK_s:
   case SDLK_a:
   case SDLK_d:
      myMovement.z = 0.0;
      myMovement.x = 0.0;
      break;
   case SDLK_UP:
   case SDLK_DOWN:
      myMovement.y = 0.0;
   default:
      break;
   }
}

void Editor::onKeyDown(SDLKey aKey)
{
   const double speed = 0.5;
   
   switch (aKey) {
   case SDLK_a:
      myMovement.z = speed;
      myMovement.x = speed;
      break;
   case SDLK_d:
      myMovement.z = -speed;
      myMovement.x = -speed;
      break;
   case SDLK_w:
      myMovement.z = speed;
      myMovement.x = -speed;
      break;
   case SDLK_s:
      myMovement.z = -speed;
      myMovement.x = speed;
      break;
   case SDLK_UP:
      myMovement.y = -speed;
      break;
   case SDLK_DOWN:
      myMovement.y = speed;
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
