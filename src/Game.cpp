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

#include "gui2/ILayout.hpp"
#include "gui2/Label.hpp"

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
   void look_ahead();
   void set_status(const string& aString) { myStatusMsg = aString; }
   void near_station(IStationPtr aStation);
   void left_station();
   Vector<float> cameraPosition(float aRadius) const;
   
   IMapPtr map;
   ITrainPtr train;
   ILightPtr sun;

   // Station the train is either approaching or stopped at
   IStationPtr active_station;

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
   ILayoutPtr layout;
   
   string myStatusMsg;
   gui::IFontPtr myStatusFont;
};

Game::Game(IMapPtr aMap)
   : map(aMap),
     myHorizAngle(2.5f), myVertAngle(1.0f), myViewRadius(10.0f),
     myCameraHTarget(2.5f), myCameraVTarget(1.0f),
     myCameraSpeed(1.0f), myCameraMode(CAMERA_FLOATING)
{
   train = makeTrain(map);
   sun = make_sun_light();

   map->setGrid(false);

   // Build the GUI
   myStatsPanel = makeFlowBox(FLOW_BOX_VERT);

   gui::IFontPtr stdFont = gui::load_font("data/fonts/Vera.ttf", 14);
   myStatusFont = gui::load_font("data/fonts/Vera.ttf", 18);
   
   mySpeedLabel = makeLabel(stdFont);
   myStatsPanel->addChild(mySpeedLabel);

   myThrottleMeter = makeThrottleMeter(stdFont);
   myStatsPanel->addChild(myThrottleMeter);

   myCoalMeter = makeFuelMeter(stdFont, "Coal",
                               make_colour(0.1f, 0.1f, 0.1f));
   //myStatsPanel->addChild(myCoalMeter);
   
   myWaterMeter = makeFuelMeter(stdFont, "Water",
                                make_colour(0.1f, 0.1f, 0.8f));
   //myStatsPanel->addChild(myWaterMeter);

   myTempLabel = makeLabel(stdFont, "Temp");
   //myStatsPanel->addChild(myTempLabel);

   myPressureLabel = makeLabel(stdFont, "Pressure");
   //myStatsPanel->addChild(myPressureLabel);

   myBrakeLabel = makeLabel(stdFont, "Brake on");
   myBrakeLabel->setColour(1.0f, 0.0f, 0.0f);
   myStatsPanel->addChild(myBrakeLabel);

   myStatsPanel->setOrigin(5, 10);

   //layout = gui::make_layout("layouts/game.xml");
}

Game::~Game()
{
   
}

Vector<float> Game::cameraPosition(float aRadius) const
{
   // Two angles give unique position on surface of a sphere
   // Look up ``spherical coordinates''
   const float yCentre = 0.9f;
   Vector<float> position = train->front();
   position.x += aRadius * cosf(myHorizAngle) * sinf(myVertAngle);
   position.z += aRadius * sinf(myHorizAngle) * sinf(myVertAngle);
   position.y = aRadius * cosf(myVertAngle) + yCentre;

   return position;
}

void Game::display(IGraphicsPtr aContext) const
{
   Vector<float> trainPos = train->front();

   Vector<float> position = cameraPosition(myViewRadius);
   
   aContext->lookAt(position, trainPos);
   setBillboardCameraOrigin(position);
   
   sun->apply();
   
   map->render(aContext);
   train->render();
}

void Game::overlay() const
{
   //myStatsPanel->render();

   //layout->render();

   const int screenH = get_game_window()->height();
   const int screenW = get_game_window()->width();
   const int len = myStatusFont->string_width("%s", myStatusMsg.c_str());
   myStatusFont->print((screenW - len)/2, screenH - 50,
                       "%s", myStatusMsg.c_str());
}

void Game::update(IPickBufferPtr aPickBuffer, int aDelta)
{
   train->update(aDelta);

   // Update the GUI elements
   const double msToMPH = 2.237;
   mySpeedLabel->setText("Speed: %.1lfmph\n", train->speed() * msToMPH);
   myThrottleMeter->setValue(train->controller()->throttle());
   myBrakeLabel->setVisible(train->controller()->brakeOn());

   //layout->get_cast<gui::Label>("/status_wnd/speed_label").format(
   //   "Speed: %.1lfmph", train->speed() * msToMPH);
   
   const double pressure = train->controller()->pressure();
   myPressureLabel->setText("Pressure: %.lfpsi", pressure);

   const double temp = train->controller()->temp();
   myTempLabel->setText("Temp: %.lfdeg", temp);

   myWaterMeter->setValue(8);

   look_ahead();

   // Move the camera vertically if it's currently underground

   // Calculate the location of the near clip plane
   const float nearClip = getConfig()->get<float>("NearClip");
   Vector<float> clipPosition = cameraPosition(myViewRadius - nearClip);

   // A hack because we don't calculate the height properly
   const float MIN_HEIGHT = 0.25f;
   float h = map->heightAt(clipPosition.x, clipPosition.z);

   if (h + MIN_HEIGHT > clipPosition.y) {    
      myCameraVTarget -= 0.001f * static_cast<float>(aDelta);
      myCameraSpeed = 200.0f;
   }
   
   // Bounce the camera if we need to
   myVertAngle -= (myVertAngle - myCameraVTarget) / myCameraSpeed;
   myHorizAngle -= (myHorizAngle - myCameraHTarget) / myCameraSpeed;
}

// Signal that we are approaching a station
void Game::near_station(IStationPtr aStation)
{
   left_station();  // Clear any previous station

   if (aStation != active_station) {
      active_station = aStation;
      aStation->setHighlightVisible(true);
   }
}

// Signal that we are no longer at or approaching a station
void Game::left_station()
{ 
   if (active_station) {
      active_station->setHighlightVisible(false);
      active_station.reset();
   }
}

// Look along the track and notify the player of any stations, points, etc.
// that they are approaching
void Game::look_ahead()
{
   TrackIterator it = iterateTrack(map, train->tile(),
                                   train->direction());

   // Are we sitting on a station?
   if (it.status == TRACK_STATION) {
      set_status("Stop here for station " + it.station->name());
      near_station(it.station);
      return;
   }

   const int maxLook = 10;
   for (int i = 0; i < maxLook; i++) {
      it = it.next();

      if (it.status != TRACK_OK) {
         bool clearStation = true;
         
         switch (it.status) {
         case TRACK_STATION:
            set_status("Approaching station " + it.station->name());
            near_station(it.station);
            clearStation = false;
            return;
         case TRACK_NO_MORE:
            set_status("Oh no! You're going to crash!");
            break;
         case TRACK_CHOICE:
            set_status("Oh no! You have to make a decision!");
            break;
         default:
            break;
         }

         if (!clearStation)
            left_station();
         return;
      }
   }
   
   // We're not approaching any station
   left_station();
   
   // Nothing to report
   set_status("");
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
      train->controller()->actOn(BRAKE_TOGGLE);
      break;
   case SDLK_LCTRL:
      train->controller()->actOn(SHOVEL_COAL);
      break;
   case SDLK_a:
      train->controller()->actOn(THROTTLE_DOWN);
      break;
   case SDLK_s:
      train->controller()->actOn(THROTTLE_UP);
      break;
   case SDLK_PRINT:
      get_game_window()->takeScreenShot();
      break;
   case SDLK_LEFT:
      train->controller()->actOn(GO_LEFT);
      break;
   case SDLK_RIGHT:
      train->controller()->actOn(GO_RIGHT);
      break;
   case SDLK_UP:
      train->controller()->actOn(GO_STRAIGHT_ON);
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
