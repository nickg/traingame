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
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y, int xrel,
      int yrel);
   void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                     MouseButton aButton);
   void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                       MouseButton aButton) {}
private:
   void lookAhead();
   void setStatus(const string& s) { statusMsg = s; }
   void nearStation(IStationPtr s);
   void leftStation();
   Vector<float> cameraPosition(float aRadius) const;
   
   IMapPtr map;
   ITrainPtr train;
   ILightPtr sun;

   // Station the train is either approaching or stopped at
   IStationPtr activeStation;

   // Camera position
   float horizAngle, vertAngle, viewRadius;

   // Camera adjustment
   float cameraHTarget, cameraVTarget;
   float cameraSpeed;

   enum CameraMode { CAMERA_FLOATING, CAMERA_FIXED, CAMERA_BIRD };
   CameraMode cameraMode;
   
   // GUI elements
   IContainerPtr myStatsPanel;
   ITextControlPtr mySpeedLabel, myBrakeLabel;
   ITextControlPtr myTempLabel, myPressureLabel;
   IMeterControlPtr myThrottleMeter;
   IMeterControlPtr myCoalMeter, myWaterMeter;
   ILayoutPtr layout;
   
   string statusMsg;
   gui::IFontPtr statusFont;
};

Game::Game(IMapPtr aMap)
   : map(aMap),
     horizAngle(2.5f), vertAngle(1.0f), viewRadius(10.0f),
     cameraHTarget(2.5f), cameraVTarget(1.0f),
     cameraSpeed(1.0f), cameraMode(CAMERA_FLOATING)
{
   train = makeTrain(map);
   sun = makeSunLight();

   map->setGrid(false);

   // Build the GUI
   myStatsPanel = makeFlowBox(FLOW_BOX_VERT);

   gui::IFontPtr stdFont = gui::load_font("data/fonts/Vera.ttf", 14);
   statusFont = gui::load_font("data/fonts/Vera.ttf", 18);
   
   mySpeedLabel = makeLabel(stdFont);
   myStatsPanel->addChild(mySpeedLabel);

   myThrottleMeter = makeThrottleMeter(stdFont);
   myStatsPanel->addChild(myThrottleMeter);

   myCoalMeter = makeFuelMeter(stdFont, "Coal",
                               makeColour(0.1f, 0.1f, 0.1f));
   //myStatsPanel->addChild(myCoalMeter);
   
   myWaterMeter = makeFuelMeter(stdFont, "Water",
                                makeColour(0.1f, 0.1f, 0.8f));
   //myStatsPanel->addChild(myWaterMeter);

   myTempLabel = makeLabel(stdFont, "Temp");
   //myStatsPanel->addChild(myTempLabel);

   myPressureLabel = makeLabel(stdFont, "Pressure");
   //myStatsPanel->addChild(myPressureLabel);

   myBrakeLabel = makeLabel(stdFont, "Brake on");
   myBrakeLabel->setColour(1.0f, 0.0f, 0.0f);
   myStatsPanel->addChild(myBrakeLabel);

   myStatsPanel->setOrigin(5, 10);

   layout = gui::makeLayout("layouts/game.xml");
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
   position.x += aRadius * cosf(horizAngle) * sinf(vertAngle);
   position.z += aRadius * sinf(horizAngle) * sinf(vertAngle);
   position.y = aRadius * cosf(vertAngle) + yCentre;

   return position;
}

void Game::display(IGraphicsPtr aContext) const
{
   Vector<float> trainPos = train->front();

   Vector<float> position = cameraPosition(viewRadius);
   
   aContext->lookAt(position, trainPos);
   setBillboardCameraOrigin(position);
   
   sun->apply();
   
   map->render(aContext);
   train->render();
}

void Game::overlay() const
{
   //myStatsPanel->render();

   layout->render();

   const int screenH = getGameWindow()->height();
   const int screenW = getGameWindow()->width();
   const int len = statusFont->string_width("%s", statusMsg.c_str());
   statusFont->print((screenW - len)/2, screenH - 50,
      "%s", statusMsg.c_str());
}

void Game::update(IPickBufferPtr aPickBuffer, int aDelta)
{
   train->update(aDelta);

   // Update the GUI elements
   const double msToMPH = 2.237;
   mySpeedLabel->setText("Speed: %.1lfmph\n", train->speed() * msToMPH);
   myThrottleMeter->setValue(train->controller()->throttle());
   myBrakeLabel->setVisible(train->controller()->brakeOn());

   layout->cast<gui::Label>("/status_wnd/speed_label").format(
      "Speed: %.1lfmph", train->speed() * msToMPH);
   
   const double pressure = train->controller()->pressure();
   myPressureLabel->setText("Pressure: %.lfpsi", pressure);

   const double temp = train->controller()->temp();
   myTempLabel->setText("Temp: %.lfdeg", temp);

   myWaterMeter->setValue(8);

   lookAhead();

   // Move the camera vertically if it's currently underground

   // Calculate the location of the near clip plane
   const float nearClip = getConfig()->get<float>("NearClip");
   Vector<float> clipPosition = cameraPosition(viewRadius - nearClip);

   // A hack because we don't calculate the height properly
   const float MIN_HEIGHT = 0.25f;
   float h = map->heightAt(clipPosition.x, clipPosition.z);

   if (h + MIN_HEIGHT > clipPosition.y) {    
      cameraVTarget -= 0.001f * static_cast<float>(aDelta);
      cameraSpeed = 200.0f;
   }
   
   // Bounce the camera if we need to
   vertAngle -= (vertAngle - cameraVTarget) / cameraSpeed;
   horizAngle -= (horizAngle - cameraHTarget) / cameraSpeed;
}

// Signal that we are approaching a station
void Game::nearStation(IStationPtr s)
{
   leftStation();  // Clear any previous station

   if (s != activeStation) {
      activeStation = s;
      s->setHighlightVisible(true);
   }
}

// Signal that we are no longer at or approaching a station
void Game::leftStation()
{ 
   if (activeStation) {
      activeStation->setHighlightVisible(false);
      activeStation.reset();
   }
}

// Look along the track and notify the player of any stations, points, etc.
// that they are approaching
void Game::lookAhead()
{
   TrackIterator it = iterateTrack(map, train->tile(),
                                   train->direction());

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
      viewRadius = max(viewRadius - 0.2f, 0.1f);
      break;
   case SDLK_PAGEDOWN:
      viewRadius += 0.2f;
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
      getGameWindow()->takeScreenShot();
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
      if (cameraMode == CAMERA_FLOATING)
         cameraMode = CAMERA_FIXED;
      else if (cameraMode == CAMERA_FIXED) {
         cameraMode = CAMERA_BIRD;

         cameraHTarget = M_PI/4.0f;
         cameraVTarget = M_PI/4.0f;

         cameraSpeed = 100.0f;
      }
      else
         cameraMode = CAMERA_FLOATING;
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
      viewRadius = max(viewRadius - 1.0f, 0.1f);
      break;
   case MOUSE_WHEEL_DOWN:
      viewRadius += 1.0f;
      break;
   default:
      break;
   }
}

void Game::onMouseMove(IPickBufferPtr aPickBuffer, int x, int y,
                       int xrel, int yrel)
{
   if (cameraMode == CAMERA_FLOATING) {
      cameraHTarget -= xrel / 100.0f;
      cameraVTarget += yrel / 100.0f;
      
      // Don't allow the camera to go under the ground
      const float ground = (M_PI / 2.0f) - 0.01f;
      if (cameraVTarget > ground)
         cameraVTarget = ground;
      
      // Don't let the camera flip over the top
      const float top = 0.01f;
      if (cameraVTarget < top)
         cameraVTarget = top;

      cameraSpeed = 2.0f;
   }
}

// Create an instance of the play screen with the given map
IScreenPtr makeGameScreen(IMapPtr aMap)
{
   return IScreenPtr(new Game(aMap));
}
