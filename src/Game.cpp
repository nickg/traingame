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

#include "IScreen.hpp"
#include "IGraphics.hpp"
#include "IMap.hpp"
#include "IRollingStock.hpp"
#include "ITrain.hpp"
#include "ILogger.hpp"
#include "ILight.hpp"
#include "gui/IContainer.hpp"

#include <GL/gl.h>

using namespace std;
using namespace gui;

// Implementation of the main play screen
class Game : public IScreen {
public:
   Game(IMapPtr aMap);
   ~Game();
   
   void display(IGraphicsPtr aContext) const;
   void overlay() const;
   void update(IPickBufferPtr aPickBuffer);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y) {}
   void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                     MouseButton aButton) {}
   void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                       MouseButton aButton) {}
private:
   IMapPtr myMap;
   ITrainPtr myTrain;
   ILightPtr mySun;

   Vector<float> myPosition;
   Vector<float> myMovement;

   // GUI elements
   IContainerPtr myStatsPanel;
   ITextControlPtr mySpeedLabel;
};

Game::Game(IMapPtr aMap)
   : myMap(aMap),
     myPosition(makeVector(16.0f, 8.0f, 16.0f))
{
   myTrain = makeTrain(myMap);
   mySun = makeSunLight();

   // Build the GUI
   myStatsPanel = makeFlowBox(FLOW_BOX_VERT);

   IFontPtr stdFont = loadFont("data/fonts/Vera.ttf", 14);
   
   mySpeedLabel = makeLabel(stdFont);
   myStatsPanel->addChild(mySpeedLabel);
}

Game::~Game()
{
   
}

void Game::display(IGraphicsPtr aContext) const
{
   Vector<float> trainPos = myTrain->front();
   aContext->lookAt(myPosition, trainPos);
   
   mySun->apply();
   
   myMap->render(aContext);
   myTrain->render();
}

void Game::overlay() const
{
   myStatsPanel->render(10, 20);
}

void Game::update(IPickBufferPtr aPickBuffer)
{
   myPosition += myMovement;

   myTrain->update();

   // Update the GUI elements
   mySpeedLabel->setText("Speed: %.1lfm/s\n", myTrain->speed());
}

void Game::onKeyDown(SDLKey aKey)
{
   const double speed = 0.5;
   const double yspeed = 0.2;
   
   switch (aKey) {
   case SDLK_a:
      myMovement.z = speed;
      //myMovement.x = speed;
      break;
   case SDLK_d:
      myMovement.z = -speed;
      //myMovement.x = -speed;
      break;
   case SDLK_w:
      //myMovement.z = speed;
      myMovement.x = -speed;
      break;
   case SDLK_s:
      //myMovement.z = -speed;
      myMovement.x = speed;
      break;
   case SDLK_UP:
      myMovement.y = yspeed;
      break;
   case SDLK_DOWN:
      myMovement.y = -yspeed;
      break;
   case SDLK_b:
      myTrain->controller()->actOn(BRAKE_TOGGLE);
      break;
   case SDLK_LCTRL:
      myTrain->controller()->actOn(SHOVEL_COAL);
      break;
   case SDLK_c:
      myTrain->controller()->actOn(THROTTLE_UP);
      break;
   case SDLK_v:
      myTrain->controller()->actOn(THROTTLE_DOWN);
      break;
   default:
      break;
   }
}

void Game::onKeyUp(SDLKey aKey)
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
      break;
   default:
      break;
   }
}

// Create an instance of the play screen with the given map
IScreenPtr makeGameScreen(IMapPtr aMap)
{
   return IScreenPtr(new Game(aMap));
}
