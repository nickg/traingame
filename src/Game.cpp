//
//  Copyright (C) 2009-2010  Nick Gasson
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
#include "GameScreens.hpp"
#include "IBillboard.hpp"
#include "IterateTrack.hpp"
#include "IConfig.hpp"
#include "IMessageArea.hpp"

#include "gui/ILayout.hpp"
#include "gui/Label.hpp"
#include "gui/ThrottleMeter.hpp"
#include "gui/IFont.hpp"

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
   void nearStation(IStationPtr s);
   void leftStation();
   Vector<float> cameraPosition(float aRadius) const;
   void switchToBirdCamera();
   void stoppedAtStation();

   enum TrackStateReq { NEXT, PREV };
   void alterTrackState(TrackStateReq req);
    
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
   
   gui::ILayoutPtr layout;
   IMessageAreaPtr messageArea;
};

Game::Game(IMapPtr aMap)
   : map(aMap),
     horizAngle(M_PI/4.0f),
     vertAngle(M_PI/4.0f),
     viewRadius(20.0f)
{
   train = makeTrain(map);
   sun = makeSunLight();

   map->setGrid(false);

   // Build the GUI
   layout = gui::makeLayout("layouts/game.xml");
   messageArea = makeMessageArea();

   switchToBirdCamera();
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

void Game::switchToBirdCamera()
{
   cameraMode = CAMERA_BIRD;

   cameraHTarget = M_PI/4.0f;
   cameraVTarget = M_PI/4.0f;

   cameraSpeed = 100.0f;
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

   renderBillboards();
}

void Game::overlay() const
{
   layout->render();
   messageArea->render();
}

void Game::stoppedAtStation()
{
   layout->get("/station").visible(true);
}

void Game::update(IPickBufferPtr aPickBuffer, int aDelta)
{
   messageArea->update(aDelta);
   
   train->update(aDelta);

   // Update the GUI elements
   layout->cast<gui::ThrottleMeter>("/throttle_meter").value(
      train->controller()->throttle());
   
   const double msToMPH = 2.237;
   layout->cast<gui::Label>("/speed_label").format(
      "Speed: %.1lfmph", train->speed() * msToMPH);

   layout->get("/brake_label").visible(train->controller()->brakeOn());
   
   lookAhead();

   // Move the camera vertically if it's currently underground
#if 0
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
#endif
   
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

      gui::Widget& stationWnd = layout->get("/station");

      layout->cast<gui::Label>("/station/name").text(s->name());
   }
}

// Signal that we are no longer at or approaching a station
void Game::leftStation()
{
   if (activeStation) {
      activeStation->setHighlightVisible(false);
      activeStation.reset();

      layout->get("/station").visible(false);
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
      nearStation(it.station);

      if (train->controller()->stopped())
         stoppedAtStation();
      else
         messageArea->post("Stop here for station " + it.station->name());
      
      return;
   }

   const int maxLook = 10;
   for (int i = 0; i < maxLook; i++) {
      it = it.next();

      if (it.status != TRACK_OK) {
         bool clearStation = true;
         
         switch (it.status) {
         case TRACK_STATION:
            messageArea->post("Approaching station " + it.station->name());
            nearStation(it.station);
            clearStation = false;
            return;
         case TRACK_NO_MORE:
            messageArea->post("Oh no! You're going to crash!");
            break;
         case TRACK_CHOICE:
            messageArea->post("Oh no! You have to make a decision!");
            it.track->setStateRenderHint();
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
}

void Game::alterTrackState(TrackStateReq req)
{
   // Change the state of the nearest points, etc.
   TrackIterator it = iterateTrack(map, train->tile(),
      train->direction());

   const int maxAlterLook = 10;

   for (int i = 0; i < maxAlterLook; i++) {

      // Skip over the first section of track which may be some
      // points - we don't want to alter the track we're on!
      it = it.next();
    
      if (it.status == TRACK_CHOICE) {
         switch (req) {
         case NEXT:
            it.track->nextState();
            break;
         case PREV:
            it.track->prevState();
            break;
         }
		
         return;
      }
   }

   warn() << "No nearby track state to change";
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
   case SDLK_r:
      if (train->controller()->throttle() == 0)
         train->controller()->actOn(TOGGLE_REVERSE);
      else
         messageArea->post("Reduce power first!", 51, 1000);
      break;
   case SDLK_LCTRL:
      train->controller()->actOn(SHOVEL_COAL);
      break;
   case SDLK_PRINT:
      getGameWindow()->takeScreenShot();
      break;
   case SDLK_LEFT:
      alterTrackState(PREV);
      break;
   case SDLK_RIGHT:
      alterTrackState(NEXT);
      break;
   case SDLK_UP:
      train->controller()->actOn(THROTTLE_UP);
      break;
   case SDLK_DOWN:
      train->controller()->actOn(THROTTLE_DOWN);
      break;
   case SDLK_TAB:
      if (cameraMode == CAMERA_FLOATING)
         cameraMode = CAMERA_FIXED;
      else if (cameraMode == CAMERA_FIXED)
         switchToBirdCamera();
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
