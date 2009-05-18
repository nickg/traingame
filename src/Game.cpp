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
#include "GameScreens.hpp"

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
   void update(IPickBufferPtr aPickBuffer, int aDelta);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y);
   void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                     MouseButton aButton);
   void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                       MouseButton aButton) {}
private:
   IMapPtr myMap;
   ITrainPtr myTrain;
   ILightPtr mySun;

   // Camera position
   float myHorizAngle, myVertAngle, myViewRadius;

   // GUI elements
   IContainerPtr myStatsPanel;
   ITextControlPtr mySpeedLabel, myBrakeLabel;
   IMeterControlPtr myThrottleMeter;
   IMeterControlPtr myCoalMeter, myWaterMeter;
   IMeterControlPtr myTempMeter, myPressureMeter;
};

Game::Game(IMapPtr aMap)
   : myMap(aMap),
     myHorizAngle(2.5f), myVertAngle(1.0f), myViewRadius(10.0f)
{
   myTrain = makeTrain(myMap);
   mySun = makeSunLight();

   myMap->setGrid(false);

   // Build the GUI
   myStatsPanel = makeFlowBox(FLOW_BOX_VERT);

   IFontPtr stdFont = loadFont("data/fonts/Vera.ttf", 14);
   
   mySpeedLabel = makeLabel(stdFont);
   myStatsPanel->addChild(mySpeedLabel);

   myThrottleMeter = makeThrottleMeter(stdFont);
   myStatsPanel->addChild(myThrottleMeter);

   myCoalMeter = makeFuelMeter(stdFont, "Coal",
                               make_tuple(0.1f, 0.1f, 0.1f));
   myStatsPanel->addChild(myCoalMeter);
   
   myWaterMeter = makeFuelMeter(stdFont, "Water",
                                make_tuple(0.1f, 0.1f, 0.8f));
   myStatsPanel->addChild(myWaterMeter);

   myTempMeter = makeFuelMeter(stdFont, "Temp",
                               make_tuple(0.8f, 0.1f, 0.1f));
   myTempMeter->setRange(0, 1050);
   myStatsPanel->addChild(myTempMeter);

   myPressureMeter = makeFuelMeter(stdFont, "Pressure",
                                   make_tuple(0.1f, 0.3f, 0.5f));
   myPressureMeter->setRange(0, 110);
   myStatsPanel->addChild(myPressureMeter);

   myBrakeLabel = makeLabel(stdFont, "Brake on");
   myBrakeLabel->setColour(1.0f, 0.0f, 0.0f);
   myStatsPanel->addChild(myBrakeLabel);

   myStatsPanel->setOrigin(5, 5);
}

Game::~Game()
{
   
}

void Game::display(IGraphicsPtr aContext) const
{
   Vector<float> trainPos = myTrain->front();

   // Two angles give unique position on surface of a sphere
   // Look up ``spherical coordinates''
   const double yCentre = 0.9f;
   Vector<float> position = trainPos;
   position.x += myViewRadius * cosf(myHorizAngle) * sinf(myVertAngle);
   position.z += myViewRadius * sinf(myHorizAngle) * sinf(myVertAngle);
   position.y = myViewRadius * cosf(myVertAngle) + yCentre;
   
   aContext->lookAt(position, trainPos);
   
   mySun->apply();
   
   myMap->render(aContext);
   myTrain->render();
}

void Game::overlay() const
{
   myStatsPanel->render();
}

void Game::update(IPickBufferPtr aPickBuffer, int aDelta)
{
   myTrain->update(aDelta);

   // Update the GUI elements
   const double msToMPH = 2.237;
   mySpeedLabel->setText("Speed: %.1lfmph\n", myTrain->speed() * msToMPH);
   myThrottleMeter->setValue(myTrain->controller()->throttle());
   myBrakeLabel->setVisible(myTrain->controller()->brakeOn());

   const double pressure = myTrain->controller()->pressure();
   myPressureMeter->setValue(static_cast<int>(pressure * 100.0));

   const double temp = myTrain->controller()->temp();
   myTempMeter->setValue(static_cast<int>(temp));

   myWaterMeter->setValue(8);
}

void Game::onKeyDown(SDLKey aKey)
{   
   switch (aKey) {
   case SDLK_PAGEUP:
      myViewRadius = max(myViewRadius - 0.2f, 0.1f);
      break;
   case SDLK_PAGEDOWN:
      myViewRadius += 0.2f;
      break;
   case SDLK_b:
      myTrain->controller()->actOn(BRAKE_TOGGLE);
      break;
   case SDLK_LCTRL:
      myTrain->controller()->actOn(SHOVEL_COAL);
      break;
   case SDLK_a:
      myTrain->controller()->actOn(THROTTLE_DOWN);
      break;
   case SDLK_s:
      myTrain->controller()->actOn(THROTTLE_UP);
      break;
   case SDLK_PRINT:
      getGameWindow()->takeScreenShot();
      break;
   default:
      break;
   }
}

void Game::onKeyUp(SDLKey aKey)
{   
 
}

void Game::onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                        MouseButton aButton)
{
   switch (aButton) {
   case MOUSE_WHEEL_UP:
      myViewRadius = max(myViewRadius - 1.0f, 0.1f);
      break;
   case MOUSE_WHEEL_DOWN:
      myViewRadius += 1.0f;
      break;
   default:
      break;
   }
}

void Game::onMouseMove(IPickBufferPtr aPickBuffer, int x, int y)
{
   static int lastX = x, lastY = y;

   const int xDelta = x - lastX;
   const int yDelta = y - lastY;
   
   myHorizAngle -= xDelta / 150.0f;
   myVertAngle += yDelta / 150.0f;

   // Don't allow the camera to go under the ground
   const double ground = (M_PI / 2.0f) - 0.01f;
   if (myVertAngle > ground)
      myVertAngle = ground;

   // Don't let the camera flip over the top
   const double top = 0.01f;
   if (myVertAngle < top)
      myVertAngle = top;

   lastX = x;
   lastY = y;
}

// Create an instance of the play screen with the given map
IScreenPtr makeGameScreen(IMapPtr aMap)
{
   return IScreenPtr(new Game(aMap));
}
