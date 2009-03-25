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

#include <GL/gl.h>

using namespace std;

// Implementation of the main play screen
class Game : public IScreen {
public:
   Game();
   
   void display(IGraphicsPtr aContext) const;
   void update(IPickBufferPtr aPickBuffer);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);
private:
   IMapPtr myMap;
   ITrainPtr myTrain;

   Vector<double> myPosition;
   Vector<double> myMovement;
   
   Vector<double> myRotation;
   Vector<double> mySpin;
};

Game::Game()
   : myPosition(makeVector(2.0, -8.0, -10.0)),
     myRotation(makeVector(45.0, 45.0, 0.0))
{
   myMap = makeEmptyMap(128, 128);
   
   myTrain = makeTrain(myMap);
}

void Game::display(IGraphicsPtr aContext) const
{
   
   aContext->setCamera(myPosition, myRotation);
   
   aContext->setAmbient(0.5, 0.5, 0.5);
   aContext->setDiffuse(0.8, 0.8, 0.8);
   aContext->moveLight(0.0, 50.0, 0.0);
   
   myTrain->render();
   myMap->render(aContext);
}

void Game::update(IPickBufferPtr aPickBuffer)
{
   myPosition += myMovement;
   myRotation += mySpin;

   myTrain->update();   
}

void Game::onKeyDown(SDLKey aKey)
{
   const double speed = 0.5;
   const double yspeed = 0.2;
   const double spinSpeed = 0.8;
   
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
      myMovement.y = -yspeed;
      break;
   case SDLK_DOWN:
      myMovement.y = yspeed;
      break;
   case SDLK_LEFT:
      mySpin.y = -spinSpeed;
      break;
   case SDLK_RIGHT:
      mySpin.y = spinSpeed;
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
   case SDLK_LEFT:
   case SDLK_RIGHT:
      mySpin.x = 0.0;
      mySpin.y = 0.0;
      break;
   default:
      break;
   }
}

// Create an instance of the play screen
IScreenPtr makeGameScreen()
{
   return IScreenPtr(new Game);
}
