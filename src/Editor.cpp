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
#include "ILight.hpp"

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
   void drawDraggedTile();
   void drawDraggedStraight(const Track::Direction& anAxis, int aLength);
   void drawDraggedCurve(int xLength, int yLength);
   bool canConnect(const Point<int>& aFirstPoint,
                   const Point<int>& aSecondPoint) const;
   void dragBoxBounds(int& xMin, int& xMax, int &yMin, int& yMax) const;
   Track::Direction guessTrackDirection();
   
   IMapPtr myMap;
   
   ILightPtr mySun;
   Vector<double> myPosition;
   Vector<double> myMovement;

   // Variables for dragging track segments
   Point<int> myDragBegin, myDragEnd;
   bool amDragging;

   // Different tools the user can be using
   enum Tool {
      TRACK_TOOL, RAISE_TOOL
   };
   Tool myTool;
};

Editor::Editor()
   : myPosition(2.0, -8.0, -10.0), amDragging(false),
     myTool(TRACK_TOOL)
{
   myMap = makeEmptyMap(32, 32);
   mySun = makeSunLight();
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
   mySun->apply();
   
   aContext->setCamera(myPosition, makeVector(45.0, 45.0, 1.0));
   
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
}

// Try to guess the axis to draw the track along by looking at nearby tiles
Track::Direction Editor::guessTrackDirection()
{
   if (canConnect(myDragBegin.left(), myDragBegin)
       || canConnect(myDragBegin.right(), myDragBegin)) {
      log() << "Connect along x";
      return Axis::X;
   }
   else if (canConnect(myDragBegin.up(), myDragBegin)
       || canConnect(myDragBegin.down(), myDragBegin)) {
      log() << "Connect along y";
      return Axis::Y;
   }
   else {
      // Take a guess
      return Axis::X;
   }
}

// Special case of drawing dragged track where the user selects just
// a single tile
void Editor::drawDraggedTile()
{
   // Look at nearby track segments and try to guess the orientation
   Track::Direction orient = guessTrackDirection();   

   myMap->setTrackAt(myDragBegin, makeStraightTrack(orient));
}

// Special case where the user drags a rectangle of width 1
// This just draws straight track along the rectangle
void Editor::drawDraggedStraight(const Track::Direction& anAxis, int aLength)
{
   Point<int> where = myDragBegin;

   log() << "drawDraggedStraight " << anAxis << " len=" << aLength;
   
   for (int i = 0; i < aLength; i++) {
      myMap->setTrackAt(where, makeStraightTrack(anAxis));
      
      where.x += anAxis.x;
      where.y += anAxis.z;
   }
}

// Connect the begin and end points of the drag with a curve and possibly
// a section of straight track
void Editor::drawDraggedCurve(int xLength, int yLength)
{
   log() << "drawDraggedCurve";

   Track::Direction dir = guessTrackDirection();

   // If we a drawing along the X axis then the track must curve to the
   // Y axis somewhere
   // If X is the longer dimension then the curve is at the end after
   // a straight section, otherwise the curve is at the beginning
   if (dir == Axis::X) {
      if (xLength > yLength) {
         log() << "Curve X->Y at end";
      }
      else {
         log() << "Curve X->Y at start";
      }
   }
   else {
      // Otherwise the track must curve to the X axis
      if (yLength > xLength) {
         log() << "Curve Y->X at end";
      }
      else {
         log() << "Curve Y->X at start";
      }
   }

   ITrackSegmentPtr curve = makeCurvedTrack();
   myMap->setTrackAt(myDragBegin, curve);
}

// Called when the user has finished dragging a rectangle for track
// Connect the beginning and end up in the simplest way possible
void Editor::drawDraggedTrack()
{
   Track::Direction straight;  // Orientation for straight track section
   
   int xmin, xmax, ymin, ymax;
   dragBoxBounds(xmin, xmax, ymin, ymax);
   
   int xlen = abs(xmax - xmin) + 1;
   int ylen = abs(ymax - ymin) + 1;
   log() << "xlen=" << xlen << ", ylen=" << ylen;
   if (xlen == 1 && ylen == 1)
      drawDraggedTile();
   else if (xlen == 1)
      drawDraggedStraight(myDragBegin.y < myDragEnd.y ? Axis::Y : -Axis::Y, ylen);
   else if (ylen == 1)
      drawDraggedStraight(myDragBegin.x < myDragEnd.x ? Axis::X : -Axis::X, xlen);
   else
      drawDraggedCurve(xlen, ylen);
   /*
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

   log() << "xlen=" << xlen << ", ylen=" << ylen;

   // Corner case where track is connected at right angles (so the
   // track doesn't actually join)
   if (straight == ALONG_X && xlen == 0 && ylen > xlen)
      straight = ALONG_Y;
   else if (straight == ALONG_Y && ylen == 0 && xlen > ylen)
      straight = ALONG_X;

   // The radius is the length of the shorter side
   // If it's equal to 1 then the track is straight
   // Otherwise it curves towards myDragEnd at the end
   int radius = (straight == ALONG_X ? ylen : xlen) + 1;
   int length = (straight == ALONG_X ? xlen : ylen) + 1;

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
      }*/

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
