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
#include "IBuilding.hpp"
#include "gui/ILayout.hpp"
#include "IBuildingPicker.hpp"

#include <algorithm>
#include <stdexcept>

#include <GL/gl.h>

// Concrete editor class
class Editor : public IScreen {
public:
   Editor(IMapPtr aMap);
   Editor(const string& aMapName);
   ~Editor();
   
   void display(IGraphicsPtr aContext) const;
   void overlay() const;
   void update(IPickBufferPtr aPickBuffer, int aDelta);
   void onKeyDown(SDLKey aKey);
   void onKeyUp(SDLKey aKey);
   void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y, int xrel, int yrel);
   void onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                     MouseButton aButton);
   void onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                       MouseButton aButton);
   
   // Different tools the user can be using
   enum Tool {
      TRACK_TOOL, RAISE_TOOL, LOWER_TOOL, DELETE_TOOL,
      LEVEL_TOOL, START_TOOL, STATION_TOOL, BUILDING_TOOL,
   };
   void setTool(Tool aTool) { myTool = aTool; }

   IMapPtr getMap() { return map; }
   void setMap(IMapPtr aMap);
   
private:
   void buildGUI();
   void drawDraggedTrack();
   bool drawTrackTile(const Point<int>& aPoint, const track::Direction& anAxis);
   void drawDraggedStraight(const track::Direction& anAxis, int aLength);
   void drawDraggedCurve(int xLength, int yLength);
   bool canConnect(const Point<int>& aFirstPoint,
                   const Point<int>& aSecondPoint) const;
   void dragBoxBounds(int& xMin, int& xMax, int &yMin, int& yMax) const;
   void deleteObjects();
   void save();
      
   IMapPtr map;
   
   ILightPtr mySun;
   Vector<float> myPosition;

   Tool myTool;
   bool amScrolling;

   // Variables for dragging track segments
   Point<int> dragBegin, dragEnd;
   bool amDragging;

   // GUI elements
   gui::ILayoutPtr layout;
   IBuildingPickerPtr buildingPicker;
};

Editor::Editor(IMapPtr aMap) 
   : map(aMap), myPosition(4.5, -17.5, -21.5),
     myTool(TRACK_TOOL), amScrolling(false), amDragging(false)
{
   mySun = makeSunLight();

   buildGUI();

   map->setGrid(true);
   
   log() << "Editing " << aMap->name();
}

Editor::~Editor()
{
   
}

void Editor::buildGUI()
{
   using namespace placeholders;
   
   layout = gui::makeLayout("layouts/editor.xml");
   
   layout->get("/tool_wnd/tools/track").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, TRACK_TOOL));
   layout->get("/tool_wnd/tools/raise").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, RAISE_TOOL));
   layout->get("/tool_wnd/tools/lower").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, LOWER_TOOL));
   layout->get("/tool_wnd/tools/level").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, LEVEL_TOOL));
   layout->get("/tool_wnd/tools/delete").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, DELETE_TOOL));
   layout->get("/tool_wnd/tools/start").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, START_TOOL));
   layout->get("/tool_wnd/tools/station").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, STATION_TOOL));
   layout->get("/tool_wnd/tools/building").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::setTool, this, BUILDING_TOOL));

   layout->get("/lower/action_wnd/save").connect(gui::Widget::SIG_CLICK,
      bind(&Editor::save, this));
   
   buildingPicker = makeBuildingPicker(layout);
}

void Editor::setMap(IMapPtr aMap)
{
   map = aMap;
   map->setGrid(true);
}

void Editor::save()
{
   map->save();
}

// Calculate the bounds of the drag box accounting for the different
// possible directions of dragging
void Editor::dragBoxBounds(int& xMin, int& xMax, int &yMin, int& yMax) const
{
   xMin = min(dragBegin.x, dragEnd.x);
   xMax = max(dragBegin.x, dragEnd.x);

   yMin = min(dragBegin.y, dragEnd.y);
   yMax = max(dragBegin.y, dragEnd.y); 
}

// Render the next frame
void Editor::display(IGraphicsPtr aContext) const
{
   if (!map)
      return;
   
   aContext->setCamera(myPosition, makeVector(45.0f, 45.0f, 0.0f));
 
   mySun->apply();
   
   map->render(aContext);

   // Draw the highlight if we are dragging track
   if (amDragging) {
      int xmin, xmax, ymin, ymax;
      dragBoxBounds(xmin, xmax, ymin, ymax);
      
      for (int x = xmin; x <= xmax; x++) {
         for (int y = ymin; y <= ymax; y++)
            map->highlightTile(aContext, makePoint(x, y),
                                 make_tuple(1.0f, 1.0f, 1.0f));
      }         
   }
}

// Render the overlay
void Editor::overlay() const
{
   layout->render();
}

// Prepare the next frame
void Editor::update(IPickBufferPtr aPickBuffer, int aDelta)
{
   
}

// True if the `aFirstPoint' is a valid track segment and it can
// connect to `aSecondPoint'
bool Editor::canConnect(const Point<int>& aFirstPoint,
                        const Point<int>& aSecondPoint) const
{
   if (!map->isValidTrack(aFirstPoint))
      return false;

   ITrackSegmentPtr track = map->trackAt(aFirstPoint);
   
   Vector<int> dir = makeVector
      (aFirstPoint.x - aSecondPoint.x, 0,
       aFirstPoint.y - aSecondPoint.y).normalise();

   return track->isValidDirection(dir)
      || track->isValidDirection(-dir);
}

// Draw a single tile of straight track and check for collisions
// Returns `false' if track cannot be placed here
bool Editor::drawTrackTile(const Point<int>& aPoint, const track::Direction& anAxis)
{
   if (map->isValidTrack(aPoint)) {
      ITrackSegmentPtr merged = map->trackAt(aPoint)->mergeExit(aPoint, anAxis);
      if (merged) {
         map->setTrackAt(aPoint, merged);
         return true;
      }
      else {
         warn() << "Cannot merge track";
         return false;
      }
   }
   else {
      map->setTrackAt(aPoint, makeStraightTrack(anAxis));
      return true;
   }
}

// Special case where the user drags a rectangle of width 1
// This just draws straight track along the rectangle
void Editor::drawDraggedStraight(const track::Direction& anAxis, int aLength)
{
   Point<int> where = dragBegin;

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
   track::Direction straight;  // Orientation for straight track section

   int xmin, xmax, ymin, ymax;
   dragBoxBounds(xmin, xmax, ymin, ymax);
   
   int xlen = abs(xmax - xmin) + 1;
   int ylen = abs(ymax - ymin) + 1;

   // Try to merge the start and end directly
   const track::Direction mergeAxis =
      xlen > ylen ? (dragBegin.x < dragEnd.x ? -axis::X : axis::X)
      : (dragBegin.y < dragEnd.y ? -axis::Y : axis::Y);
   if (map->isValidTrack(dragEnd)) {
      ITrackSegmentPtr merged =
         map->trackAt(dragEnd)->mergeExit(dragBegin, mergeAxis);

      if (merged) {
         // Erase all the tiles covered
         for (int x = xmin; x <= xmax; x++) {
            for (int y = ymin; y <= ymax; y++)
               map->eraseTile(x, y);
         }
         
         map->setTrackAt(dragEnd, merged);
         return;
      }
   }
      
   // Normalise the coordinates so the start is always the one with
   // the smallest x-coordinate
   if (dragBegin.x > dragEnd.x)
      swap(dragBegin, dragEnd);

   track::Direction startDir, endDir;
   bool startWasGuess = false;
   bool endWasGuess = false;

   // Try to work out the direction of the track start
   if (canConnect(dragBegin.left(), dragBegin)
       || canConnect(dragBegin.right(), dragBegin)) {
      startDir = axis::X;
   }
   else if (canConnect(dragBegin.up(), dragBegin)
       || canConnect(dragBegin.down(), dragBegin)) {
      startDir = axis::Y;
   }
   else
      startWasGuess = true;

   // Try to work out the direction of the track end
   if (canConnect(dragEnd.left(), dragEnd)
       || canConnect(dragEnd.right(), dragEnd)) {
      endDir = axis::X;
   }
   else if (canConnect(dragEnd.up(), dragEnd)
       || canConnect(dragEnd.down(), dragEnd)) {
      endDir = axis::Y;
   }
   else
      endWasGuess = true;

   // If we have to guess both orientations use a heuristic to decide
   // between S-bends and curves
   if (endWasGuess && startWasGuess) {
      if (min(xlen, ylen) <= 2) {
         if (xlen > ylen)
            startDir = endDir = axis::X;
         else
            startDir = endDir = axis::Y;
      }
      else {
         startDir = axis::X;
         endDir = axis::Y;
      }
   }
   // Otherwise always prefer curves to S-bends
   else if (startWasGuess)
      startDir = endDir == axis::X ? axis::Y : axis::X;
   else if (endWasGuess)
      endDir = startDir == axis::X ? axis::Y : axis::X;
   
   if (xlen == 1 && ylen == 1) {
      // A single tile
      drawTrackTile(dragBegin, startDir);
   }
   else if (xlen == 1)
      drawDraggedStraight(dragBegin.y < dragEnd.y ? axis::Y : -axis::Y, ylen);
   else if (ylen == 1)
      drawDraggedStraight(axis::X, xlen);
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
            if (startDir == axis::X) {
               drawTrackTile(dragBegin, axis::X);
               dragBegin.x++;
            }
            else {
               drawTrackTile(dragEnd, axis::X);
               dragEnd.x--;
            }
            xlen--;
         }
         else {
            // Need to draw track along y-axis
            if (startDir == axis::Y) {
               drawTrackTile(dragBegin, axis::Y);

               // The y-coordinate for the drag points is not guaranteed
               // to be sorted
               if (dragBegin.y > dragEnd.y)
                  dragBegin.y--;
               else
                  dragBegin.y++;
            }
            else {
               drawTrackTile(dragEnd, axis::Y);
                                    
               if (dragBegin.y > dragEnd.y)
                  dragEnd.y++;
               else
                  dragEnd.y--;
            }
            ylen--;
         }
      }

      track::Angle startAngle, endAngle;
      Point<int> where;

      if (startDir == axis::X && endDir == axis::Y) {
         if (dragBegin.y < dragEnd.y) {
            startAngle = 90;
            endAngle = 180;
            where = dragEnd;
         }
         else {
            startAngle = 0;
            endAngle = 90;
            where = dragBegin;
         }
      }
      else {
         if (dragBegin.y < dragEnd.y) {
            startAngle = 270;
            endAngle = 360;
            where = dragBegin;
         }
         else {
            startAngle = 180;
            endAngle = 270;
            where = dragEnd;
         }
      }

      ITrackSegmentPtr track = makeCurvedTrack(startAngle, endAngle, xlen);
      track->setOrigin(where.x, where.y);

      list<Point<int> > exits;
      track->getEndpoints(exits);

      bool ok = true;
      for (list<Point<int> >::iterator it = exits.begin();
           it != exits.end(); ++it) {
         if (map->isValidTrack(*it)) {
            warn() << "Cannot place curve here";
            ok = false;
            break;
         }
      }

      if (ok)
         map->setTrackAt(where, track);
   }
}

// Delete all objects in the area selected by the user
void Editor::deleteObjects()
{
   int xmin, xmax, ymin, ymax;
   dragBoxBounds(xmin, xmax, ymin, ymax);

   for (int x = xmin; x <= xmax; x++) {
      for (int y = ymin; y <= ymax; y++)
         map->eraseTile(x, y);
   }
}

void Editor::onMouseMove(IPickBufferPtr aPickBuffer, int x, int y,
                         int xrel, int yrel)
{
   if (amDragging) {
      // Extend the selection rectangle
      map->setPickMode(true);
      IGraphicsPtr pickContext = aPickBuffer->beginPick(x, y);
      display(pickContext);
      int id = aPickBuffer->endPick();
      map->setPickMode(false);

      if (id > 0)
         dragEnd = map->pickPosition(id);
   }
   else if (amScrolling) {
      const float speed = 0.05f;
      
      myPosition.x -= xrel * speed;
      myPosition.z -= xrel * speed;
      
      myPosition.x += yrel * speed;
      myPosition.z -= yrel * speed;      
   }
}

void Editor::onMouseClick(IPickBufferPtr aPickBuffer, int x, int y,
                          MouseButton aButton)
{   
   if (aButton == MOUSE_RIGHT) {
      // Start scrolling
      amScrolling = true;
   }
   else if (aButton == MOUSE_LEFT) {
      bool clickedOnGUI = layout->click(x, y);

      if (!clickedOnGUI) {
         // See if the user clicked on something in the map
         map->setPickMode(true);
         IGraphicsPtr pickContext = aPickBuffer->beginPick(x, y);
         display(pickContext);
         int id = aPickBuffer->endPick();
         map->setPickMode(false);
         
         if (id > 0) {
            // Begin dragging a selection rectangle
            Point<int> where = map->pickPosition(id);
         
            dragBegin = dragEnd = where;
            amDragging = true;
         }
      }
   }
   else if (aButton == MOUSE_WHEEL_UP) {
      myPosition.y -= 0.5;
   }
   else if (aButton == MOUSE_WHEEL_DOWN) {
      myPosition.y += 0.5;
   }
}

void Editor::onMouseRelease(IPickBufferPtr aPickBuffer, int x, int y,
                            MouseButton aButton)
{
   if (amDragging) {
      // Stop dragging and perform the action
      switch (myTool) {
      case TRACK_TOOL:
         drawDraggedTrack();
         break;
      case RAISE_TOOL:
         map->raiseArea(dragBegin, dragEnd);
         break;
      case LOWER_TOOL:
         map->lowerArea(dragBegin, dragEnd);
         break;
      case LEVEL_TOOL:
         map->levelArea(dragBegin, dragEnd);
         break;
      case DELETE_TOOL:
         deleteObjects();
         break;
      case START_TOOL:
         map->setStart(dragBegin.x, dragBegin.y);
         break;
      case STATION_TOOL:
         map->extendStation(dragBegin, dragEnd);
         break;
      case BUILDING_TOOL:
         {
            float angle;
            IBuildingPtr building;
            tie(building, angle) = buildingPicker->get();
            map->placeBuilding(dragBegin, building, angle);
         }
         break;
      }
         
      amDragging = false;
   }
   else if (amScrolling) {
      amScrolling = false;
   }
}

void Editor::onKeyUp(SDLKey aKey)
{
   
}

void Editor::onKeyDown(SDLKey aKey)
{
   switch (aKey) {
   case SDLK_g:
      // Toggle grid
      map->setGrid(true);
      break;
   default:
      break;
   }
}

// Create an instance of the editor screen
IScreenPtr makeEditorScreen(IMapPtr aMap)
{
   return IScreenPtr(new Editor(aMap));
}
