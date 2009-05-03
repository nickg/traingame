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
#include "gui/IContainer.hpp"

#include <algorithm>

#include <GL/gl.h>

using namespace std;
using namespace gui;

// Concrete editor class
class Editor : public IScreen {
public:
   Editor(IMapPtr aMap, const string& aFileName);
   ~Editor();
   
   void display(IGraphicsPtr aContext) const;
   void overlay() const;
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
   bool drawTrackTile(const Point<int>& aPoint, const Track::Direction& anAxis);
   void drawDraggedStraight(const Track::Direction& anAxis, int aLength);
   void drawDraggedCurve(int xLength, int yLength);
   bool canConnect(const Point<int>& aFirstPoint,
                   const Point<int>& aSecondPoint) const;
   void dragBoxBounds(int& xMin, int& xMax, int &yMin, int& yMax) const;
   
   IMapPtr myMap;
   
   ILightPtr mySun;
   Vector<double> myPosition;
   Vector<double> myMovement;

   string myFileName;

   // Variables for dragging track segments
   Point<int> myDragBegin, myDragEnd;
   bool amDragging;

   // Different tools the user can be using
   enum Tool {
      TRACK_TOOL, RAISE_TOOL
   };
   Tool myTool;

   // GUI variables
   IContainerPtr myToolbar;
};

Editor::Editor(IMapPtr aMap, const string& aFileName)
   : myMap(aMap), myPosition(4.5, -15.0, -21.5),
     myFileName(aFileName), amDragging(false), myTool(TRACK_TOOL)
{
   mySun = makeSunLight();

   // Build the GUI
   myToolbar = makeFlowBox(FLOW_BOX_HORIZ);

   IControlPtr trackButton =
      makeButton(loadTexture("data/images/track_icon.png"));
   myToolbar->addChild(trackButton);

   myMap->setGrid(true);

   log() << "Editing " << aFileName;
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
   aContext->setCamera(myPosition, makeVector(45.0, 45.0, 0.0));
 
   mySun->apply();
   
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

// Render the overlay
void Editor::overlay() const
{
   myToolbar->render();
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

   return track->isValidDirection(dir)
      || track->isValidDirection(-dir);
}

// Draw a single tile of straight track and check for collisions
// Returns `false' if track cannot be placed here
bool Editor::drawTrackTile(const Point<int>& aPoint, const Track::Direction& anAxis)
{
   if (myMap->isValidTrack(aPoint)) {
      ITrackSegmentPtr merged = myMap->trackAt(aPoint)->mergeExit(aPoint, anAxis);
      if (merged) {
         myMap->setTrackAt(aPoint, merged);
         return true;
      }
      else {
         warn() << "Cannot merge track";
         return false;
      }
   }
   else {
      myMap->setTrackAt(aPoint, makeStraightTrack(anAxis));
      return true;
   }
}

// Special case where the user drags a rectangle of width 1
// This just draws straight track along the rectangle
void Editor::drawDraggedStraight(const Track::Direction& anAxis, int aLength)
{
   Point<int> where = myDragBegin;

   for (int i = 0; i < aLength; i++) {
      drawTrackTile(where, anAxis);
      
      where.x += anAxis.x;
      where.y += anAxis.z;
   }
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
   // Normalise the coordinates so the start is always the one with
   // the smallest x-coordinate
   if (myDragBegin.x > myDragEnd.x)
      swap(myDragBegin, myDragEnd);

   Track::Direction startDir, endDir;
   bool startWasGuess = false;
   bool endWasGuess = false;

   // Try to work out the direction of the track start
   if (canConnect(myDragBegin.left(), myDragBegin)
       || canConnect(myDragBegin.right(), myDragBegin)) {
      startDir = Axis::X;
   }
   else if (canConnect(myDragBegin.up(), myDragBegin)
       || canConnect(myDragBegin.down(), myDragBegin)) {
      startDir = Axis::Y;
   }
   else
      startWasGuess = true;

   // Try to work out the direction of the track end
   if (canConnect(myDragEnd.left(), myDragEnd)
       || canConnect(myDragEnd.right(), myDragEnd)) {
      endDir = Axis::X;
   }
   else if (canConnect(myDragEnd.up(), myDragEnd)
       || canConnect(myDragEnd.down(), myDragEnd)) {
      endDir = Axis::Y;
   }
   else
      endWasGuess = true;

   // If we have to guess both orientations use a heuristic to decide
   // between S-bends and curves
   if (endWasGuess && startWasGuess) {
      if (min(xlen, ylen) <= 2) {
         if (xlen > ylen)
            startDir = endDir = Axis::X;
         else
            startDir = endDir = Axis::Y;
      }
      else {
         startDir = Axis::X;
         endDir = Axis::Y;
      }
   }
   // Otherwise always prefer curves to S-bends
   else if (startWasGuess)
      startDir = endDir == Axis::X ? Axis::Y : Axis::X;
   else if (endWasGuess)
      endDir = startDir == Axis::X ? Axis::Y : Axis::X;
   
   if (xlen == 1 && ylen == 1) {
      // A single tile
      drawTrackTile(myDragBegin, startDir);
   }
   else if (xlen == 1)
      drawDraggedStraight(myDragBegin.y < myDragEnd.y ? Axis::Y : -Axis::Y, ylen);
   else if (ylen == 1)
      drawDraggedStraight(Axis::X, xlen);
   else if (startDir == endDir) {
      // An S-bend (not implemented)
      warn() << "Sorry! No S-bends yet...";
   }
   else {
      // Curves at the moment cannot be ellipses so lay track down
      // until the dragged area is a square
      while (xlen != ylen) {
         if (xlen > ylen) {
            // One of the ends must lie along the x-axis since all
            // curves are through 90 degrees so extend that one
            if (startDir == Axis::X) {
               drawTrackTile(myDragBegin, Axis::X);
               myDragBegin.x++;
            }
            else {
               drawTrackTile(myDragEnd, Axis::X);
               myDragEnd.x--;
            }
            xlen--;
         }
         else {
            // Need to draw track along y-axis
            if (startDir == Axis::Y) {
               drawTrackTile(myDragBegin, Axis::Y);

               // The y-coordinate for the drag points is not guaranteed
               // to be sorted
               if (myDragBegin.y > myDragEnd.y)
                  myDragBegin.y--;
               else
                  myDragBegin.y++;
            }
            else {
               drawTrackTile(myDragEnd, Axis::Y);
                                    
               if (myDragBegin.y > myDragEnd.y)
                  myDragEnd.y++;
               else
                  myDragEnd.y--;
            }
            ylen--;
         }
      }

      float startAngle, endAngle;
      Point<int> where;

      if (startDir == Axis::X && endDir == Axis::Y) {
         if (myDragBegin.y < myDragEnd.y) {
            startAngle = 90;
            endAngle = 180;
            where = myDragEnd;
         }
         else {
            startAngle = 0;
            endAngle = 90;
            where = myDragBegin;
         }
      }
      else {
         if (myDragBegin.y < myDragEnd.y) {
            startAngle = 270;
            endAngle = 360;
            where = myDragBegin;
         }
         else {
            startAngle = 180;
            endAngle = 270;
            where = myDragEnd;
         }
      }

      ITrackSegmentPtr track = makeCurvedTrack(startAngle, endAngle, xlen);
      track->setOrigin(where.x, where.y);

      list<Point<int> > exits;
      track->getEndpoints(exits);

      bool ok = true;
      for (list<Point<int> >::iterator it = exits.begin();
           it != exits.end(); ++it) {
         if (myMap->isValidTrack(*it)) {
            warn() << "Cannot place curve here";
            ok = false;
            break;
         }
      }

      if (ok)
         myMap->setTrackAt(where, track);
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
   case SDLK_x:
      // Write out to disk
      myMap->save(myFileName);
      break;
   case SDLK_g:
      // Toggle grid
      myMap->setGrid(true);
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
IScreenPtr makeEditorScreen(IMapPtr aMap, const string& aFileName)
{
   return IScreenPtr(new Editor(aMap, aFileName));
}
