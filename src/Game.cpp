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
#include "IBillboard.hpp"
#include "IterateTrack.hpp"
#include "IConfig.hpp"

#include <GL/gl.h>

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
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y, int xrel, int yrel);
   void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                     MouseButton aButton);
   void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                       MouseButton aButton) {}
private:
   void lookAhead();
   void setStatus(const string& aString) { myStatusMsg = aString; }
   void nearStation(IStationPtr aStation);
   void leftStation();
   Vector<float> cameraPosition(float aRadius) const;
   
   IMapPtr myMap;
   ITrainPtr myTrain;
   ILightPtr mySun;

   // Station the train is either approaching or stopped at
   IStationPtr myActiveStation;

   // Camera position
   float myHorizAngle, myVertAngle, myViewRadius;

   // Camera adjustment
   float myCameraHTarget, myCameraVTarget;
   float myCameraSpeed;

   enum CameraMode { CAMERA_FLOATING, CAMERA_FIXED, CAMERA_BIRD };
   CameraMode myCameraMode;
   
   // GUI elements
   IContainerPtr myStatsPanel;
   ITextControlPtr mySpeedLabel, myBrakeLabel;
   ITextControlPtr myTempLabel, myPressureLabel;
   IMeterControlPtr myThrottleMeter;
   IMeterControlPtr myCoalMeter, myWaterMeter;

   string myStatusMsg;
   IFontPtr myStatusFont;
};

Game::Game(IMapPtr aMap)
   : myMap(aMap),
     myHorizAngle(2.5f), myVertAngle(1.0f), myViewRadius(10.0f),
     myCameraHTarget(2.5f), myCameraVTarget(1.0f),
     myCameraSpeed(1.0f), myCameraMode(CAMERA_FLOATING)
{
   myTrain = makeTrain(myMap);
   mySun = makeSunLight();

   myMap->setGrid(false);

   // Build the GUI
   myStatsPanel = makeFlowBox(FLOW_BOX_VERT);

   IFontPtr stdFont = load_font("data/fonts/Vera.ttf", 14);
   myStatusFont = load_font("data/fonts/Vera.ttf", 18);
   
   mySpeedLabel = makeLabel(stdFont);
   myStatsPanel->addChild(mySpeedLabel);

   myThrottleMeter = makeThrottleMeter(stdFont);
   myStatsPanel->addChild(myThrottleMeter);

   myCoalMeter = makeFuelMeter(stdFont, "Coal",
                               make_tuple(0.1f, 0.1f, 0.1f));
   //myStatsPanel->addChild(myCoalMeter);
   
   myWaterMeter = makeFuelMeter(stdFont, "Water",
                                make_tuple(0.1f, 0.1f, 0.8f));
   //myStatsPanel->addChild(myWaterMeter);

   myTempLabel = makeLabel(stdFont, "Temp");
   //myStatsPanel->addChild(myTempLabel);

   myPressureLabel = makeLabel(stdFont, "Pressure");
   //myStatsPanel->addChild(myPressureLabel);

   myBrakeLabel = makeLabel(stdFont, "Brake on");
   myBrakeLabel->setColour(1.0f, 0.0f, 0.0f);
   myStatsPanel->addChild(myBrakeLabel);

   myStatsPanel->setOrigin(5, 10);
}

Game::~Game()
{
   
}

Vector<float> Game::cameraPosition(float aRadius) const
{
   // Two angles give unique position on surface of a sphere
   // Look up ``spherical coordinates''
   const float yCentre = 0.9f;
   Vector<float> position = myTrain->front();
   position.x += aRadius * cosf(myHorizAngle) * sinf(myVertAngle);
   position.z += aRadius * sinf(myHorizAngle) * sinf(myVertAngle);
   position.y = aRadius * cosf(myVertAngle) + yCentre;

   return position;
}

void Game::display(IGraphicsPtr aContext) const
{
   Vector<float> trainPos = myTrain->front();

   Vector<float> position = cameraPosition(myViewRadius);
   
   aContext->lookAt(position, trainPos);
   setBillboardCameraOrigin(position);
   
   mySun->apply();
   
   myMap->render(aContext);
   myTrain->render();
}

void Game::overlay() const
{
   myStatsPanel->render();

   const int screenH = getGameWindow()->height();
   const int screenW = getGameWindow()->width();
   const int len = myStatusFont->string_width("%s", myStatusMsg.c_str());
   myStatusFont->print((screenW - len)/2, screenH - 50,
                       "%s", myStatusMsg.c_str());
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
   myPressureLabel->setText("Pressure: %.lfpsi", pressure);

   const double temp = myTrain->controller()->temp();
   myTempLabel->setText("Temp: %.lfdeg", temp);

   myWaterMeter->setValue(8);

   lookAhead();

   // Move the camera vertically if it's currently underground

   // Calculate the location of the near clip plane
   const float nearClip = getConfig()->get<float>("NearClip");
   Vector<float> clipPosition = cameraPosition(myViewRadius - nearClip);

   // A hack because we don't calculate the height properly
   const float MIN_HEIGHT = 0.25f;
   float h = myMap->heightAt(clipPosition.x, clipPosition.z);

   if (h + MIN_HEIGHT > clipPosition.y) {    
      myCameraVTarget -= 0.001f * static_cast<float>(aDelta);
      myCameraSpeed = 200.0f;
   }
   
   // Bounce the camera if we need to
   myVertAngle -= (myVertAngle - myCameraVTarget) / myCameraSpeed;
   myHorizAngle -= (myHorizAngle - myCameraHTarget) / myCameraSpeed;
}

// Signal that we are approaching a station
void Game::nearStation(IStationPtr aStation)
{
   leftStation();  // Clear any previous station

   if (aStation != myActiveStation) {
      myActiveStation = aStation;
      aStation->setHighlightVisible(true);
   }
}

// Signal that we are no longer at or approaching a station
void Game::leftStation()
{ 
   if (myActiveStation) {
      myActiveStation->setHighlightVisible(false);
      myActiveStation.reset();
   }
}

// Look along the track and notify the player of any stations, points, etc.
// that they are approaching
void Game::lookAhead()
{
   TrackIterator it = iterateTrack(myMap, myTrain->tile(),
                                   myTrain->direction());

   // Are we sitting on a station?
   if (it.status == TRACK_STATION) {
      setStatus("Stop here for station " + it.station->name());
      nearStation(it.station);
      return;
   }

   const int maxLook = 10;
   for (int i = 0; i < maxLook; i++) {
      it = it.next();

      if (it.status != TRACK_OK) {
         bool clearStation = true;
         
         switch (it.status) {
         case TRACK_STATION:
            setStatus("Approaching station " + it.station->name());
            nearStation(it.station);
            clearStation = false;
            return;
         case TRACK_NO_MORE:
            setStatus("Oh no! You're going to crash!");
            break;
         case TRACK_CHOICE:
            setStatus("Oh no! You have to make a decision!");
            break;
         default:
            break;
         }

         if (!clearStation)
            leftStation();
         return;
      }
   }
   
   // We're not approaching any station
   leftStation();
   
   // Nothing to report
   setStatus("");
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
   case SDLK_LEFT:
      myTrain->controller()->actOn(GO_LEFT);
      break;
   case SDLK_RIGHT:
      myTrain->controller()->actOn(GO_RIGHT);
      break;
   case SDLK_UP:
      myTrain->controller()->actOn(GO_STRAIGHT_ON);
      break;
   case SDLK_TAB:
      if (myCameraMode == CAMERA_FLOATING)
         myCameraMode = CAMERA_FIXED;
      else if (myCameraMode == CAMERA_FIXED) {
         myCameraMode = CAMERA_BIRD;

         myCameraHTarget = M_PI/4.0f;
         myCameraVTarget = M_PI/4.0f;

         myCameraSpeed = 100.0f;
      }
      else
         myCameraMode = CAMERA_FLOATING;
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

void Game::onMouseMove(IPickBufferPtr aPickBuffer, int x, int y,
                       int xrel, int yrel)
{
   if (myCameraMode == CAMERA_FLOATING) {
      myCameraHTarget -= xrel / 100.0f;
      myCameraVTarget += yrel / 100.0f;
      
      // Don't allow the camera to go under the ground
      const float ground = (M_PI / 2.0f) - 0.01f;
      if (myCameraVTarget > ground)
         myCameraVTarget = ground;
      
      // Don't let the camera flip over the top
      const float top = 0.01f;
      if (myCameraVTarget < top)
         myCameraVTarget = top;

      myCameraSpeed = 2.0f;
   }
}

// Create an instance of the play screen with the given map
IScreenPtr makeGameScreen(IMapPtr aMap)
{
   return IScreenPtr(new Game(aMap));
}
