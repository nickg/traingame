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

#include <algorithm>

#include <GL/gl.h>

using namespace std;

// Concrete editor class
class Editor : public IScreen {
public:
   Editor();
   ~Editor();
   
   void display(IGraphicsPtr aContext) const;
   void update(IPickBufferPtr aPickBuffer);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y);
   void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                     MouseButton aButton);
   void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                       MouseButton aButton);
private:
   void buildGUI();
   void drawDraggedTrack();
   bool canConnect(const Point<int>& aFirstPoint,
                   const Point<int>& aSecondPoint) const;
   void dragBoxBounds(int& xMin, int& xMax, int &yMin, int& yMax) const;
   
   IMapPtr myMap;

   Vector<double> myPosition;
   Vector<double> myMovement;

   // Variables for dragging track segments
   Point<int> myDragBegin, myDragEnd;
   bool amDragging;
};

Editor::Editor()
   : myPosition(2.0, -8.0, -10.0), amDragging(false)
{
   myMap = makeEmptyMap(32, 32);
}

Editor::~Editor()
{
   
}

// Calculate the bounds of the drag box accounting for the different
// possible directions of dragging
void Editor::dragBoxBounds(int& xMin, int& xMax, int &yMin, int& yMax) const
{
   xMin = min(myDragBegin.x, myDragEnd.x);
   xMax = max(myDragBegin.x, myDragEnd.x);

   yMin = min(myDragBegin.y, myDragEnd.y);
   yMax = max(myDragBegin.y, myDragEnd.y); 
}
   
// Render the next frame
void Editor::display(IGraphicsPtr aContext) const
{
   aContext->setCamera(myPosition, makeVector(45.0, 45.0, 1.0));
   
   aContext->setAmbient(0.5, 0.5, 0.5);
   aContext->setDiffuse(0.8, 0.8, 0.8);
   aContext->moveLight(0.0, 50.0, 0.0);

   myMap->render(aContext);

   // Draw the highlight if we are dragging track
   if (amDragging) {
      int xmin, xmax, ymin, ymax;
      dragBoxBounds(xmin, xmax, ymin, ymax);
      
      for (int x = xmin; x <= xmax; x++) {
         for (int y = ymin; y <= ymax; y++)
            myMap->highlightTile(aContext, makePoint(x, y));
      }         
   }
}

// Prepare the next frame
void Editor::update(IPickBufferPtr aPickBuffer)
{
   myPosition += myMovement;
}

// True if the `aFirstPoint' is a valid track segment and it can
// connect to `aSecondPoint'
bool Editor::canConnect(const Point<int>& aFirstPoint,
                        const Point<int>& aSecondPoint) const
{
   if (!myMap->isValidTrack(aFirstPoint))
      return false;

   ITrackSegmentPtr track = myMap->trackAt(aFirstPoint);
   
   Vector<int> dir = makeVector
      (aFirstPoint.x - aSecondPoint.x, 0,
       aFirstPoint.y - aSecondPoint.y).normalise();

   log() << dir << ", " << -dir;

   return (track->isValidDirection(dir)
           && track->nextPosition(dir).first == aSecondPoint)
      || (track->isValidDirection(-dir)
          && track->nextPosition(-dir).first == aSecondPoint);
   
   /*if () {
      log() << track->nextPosition(dir).first
            << " == " << aSecondPoint;
      return ;
   }
   else if ()
      return ;
   else
   return false;*/
}

// Called when the user has finished dragging a rectangle for track
// Connect the beginning and end up in the simplest way possible
void Editor::drawDraggedTrack()
{
   Orientation straight;  // Orientation for straight track section
   
   int xmin, xmax, ymin, ymax;
   dragBoxBounds(xmin, xmax, ymin, ymax);
   
   int xlen = abs(xmax - xmin);
   int ylen = abs(ymax - ymin);

   // Try to guess the initial orientation from a nearby track segment
   if (canConnect(myDragBegin.left(), myDragBegin)
       || canConnect(myDragBegin.right(), myDragBegin)) {
      log() << "Connect along x";
      straight = ALONG_X;
   }
   else if (canConnect(myDragBegin.up(), myDragBegin)
       || canConnect(myDragBegin.down(), myDragBegin)) {
      log() << "Connect along y";
      straight = ALONG_Y;
   }
   else {
      // There isn't an adjoining track segment to connect to
      // Guess that the user wants to connect along the longest axis
      if (xlen > ylen) {
         log() << "(Guess) connect along x";
         straight = ALONG_X;
      }
      else {
         log() << "(Guess) connect along y";
         straight = ALONG_Y;
      }
   }

   // The radius is the length of the shorter side
   // If it's equal to 1 then the track is straight
   // Otherwise it curves towards myDragEnd at the end
   int radius = (xlen > ylen ? ylen : xlen) + 1;
   int length = (xlen > ylen ? xlen : ylen) + 1;

   // The length of the straight track until it starts to curve away
   int straightLen = radius == 1 ? length : length - radius;

   log() << "radius = " << radius << ", straightLen = " << straightLen;

   // The direction to go from the start to the end
   int dir;
   if (straight == ALONG_X)
      dir = myDragBegin.x < myDragEnd.x ? 1 : -1;
   else
      dir = myDragBegin.y < myDragEnd.y ? 1 : -1;

   // Make the straight part
   for (int i = 0; i < straightLen; i++) {
      Point<int> where = straight == ALONG_X
         ? makePoint(myDragBegin.x + i*dir, myDragBegin.y)
         : makePoint(myDragBegin.x, myDragBegin.y + i*dir);

      myMap->setTrackAt(where, makeStraightTrack(straight));
   }

   myMap->rebuildDisplayLists();
}

void Editor::onMouseMove(IPickBufferPtr aPickBuffer, int x, int y)
{
   if (amDragging) {
      // Extend the selection rectangle
      IGraphicsPtr pickContext = aPickBuffer->beginPick(x, y);
      display(pickContext);
      int id = aPickBuffer->endPick();

      if (id > 0)
         myDragEnd = myMap->pickPosition(id);
   }      
}

void Editor::onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                          MouseButton aButton)
{
   IGraphicsPtr pickContext = aPickBuffer->beginPick(x, y);
   display(pickContext);
   int id = aPickBuffer->endPick();

   if (aButton == MOUSE_LEFT && id > 0) {
      // Begin dragging a selection rectangle
      Point<int> where = myMap->pickPosition(id);
      
      myDragBegin = myDragEnd = where;
      amDragging = true;
   }
}

void Editor::onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                            MouseButton aButton)
{
   if (amDragging) {
      // Stop dragging and draw the track
      drawDraggedTrack();
      amDragging = false;
   }
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
   case SDLK_p:
      // Switch to play mode
      {
         IScreenPtr game = makeGameScreen(myMap);
         getGameWindow()->switchScreen(game);
      }
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
