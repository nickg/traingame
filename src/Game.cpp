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
   
   void display(IGraphicsPtr aContext);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);
private:
   IMapPtr myMap;
   ITrainPtr myTrain;
};

Game::Game()
{
   myMap = makeEmptyMap(128, 128);
   
   myTrain = makeTrain(myMap);
}

void Game::display(IGraphicsPtr aContext)
{
   myTrain->update();
   
   glRotated(0.0, 1.0, 0.0, 0.0);
   glRotated(0.0, 0.0, 1.0, 0.0);
   aContext->setCamera(makeVector(-1.0, -1.0, -10.0));

   aContext->setAmbient(0.5, 0.5, 0.5);
   aContext->setDiffuse(0.8, 0.8, 0.8);
   aContext->moveLight(0.0, 50.0, 0.0);
   
   myTrain->render();
   myMap->render(aContext);
}

void Game::onKeyDown(SDLKey aKey)
{

}

void Game::onKeyUp(SDLKey aKey)
{
   
}

// Create an instance of the play screen
IScreenPtr makeGameScreen()
{
   return IScreenPtr(new Game);
}
