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

#include "ITrain.hpp"
#include "IRollingStock.hpp"
#include "ILogger.hpp"

#include <stdexcept>

#include <GL/gl.h>

using namespace std;

// Concrete implementation of trains
class Train : public ITrain {
public:
   Train(IMapPtr aMap);
   
   void render() const;
   void update();
private:
   IRollingStockPtr myEngine;
   IMapPtr myMap;

   // The length of a track segment can be found by calling segmentLength()
   // This delta value ranges from 0 to that length and indicates how far
   // along the segment the train is
   ITrackSegmentPtr mySegment;
   double mySegmentDelta;

   // This updates the above two values
   void enterSegment(const Point<int>& aPoint);

   static const double MODEL_SCALE;
   static const double MODEL_YOFF;
};

const double Train::MODEL_SCALE(0.4);
const double Train::MODEL_YOFF(0.05);

Train::Train(IMapPtr aMap)
   : myMap(aMap)
{
   enterSegment(aMap->startLocation());

   myEngine = makeEngine();
}

// Move the train along the line a bit
void Train::update()
{
   mySegmentDelta += 0.00;
   
   if (mySegmentDelta >= mySegment->segmentLength()) {
      // Moved onto a new piece of track
      Point<int> next = mySegment->nextPosition();

      if (!myMap->isValidTrack(next))
         throw runtime_error("Train fell off end of track!");

      enterSegment(next);
   }
}

// Called when the train enters a new segment
// Resets the delta and gets the length of the new segment
void Train::enterSegment(const Point<int>& aPoint)
{
   debug() << "Train entered segment at " << aPoint;
   
   mySegmentDelta = 0.0;
   mySegment = myMap->trackAt(aPoint);
}

void Train::render() const
{
   glPushMatrix();

   Vector<double> loc = mySegment->offsetForDelta(mySegmentDelta);
   glTranslated(loc.x, loc.y + MODEL_YOFF, loc.z);
   glRotated(-90.0, 0.0, 1.0, 0.0);

   // All rolling stock is scaled by a uniform amount
   glScaled(MODEL_SCALE, MODEL_SCALE, MODEL_SCALE);
   
   myEngine->render();

   glPopMatrix();
}

// Make an empty train
ITrainPtr makeTrain(IMapPtr aMap)
{
   return ITrainPtr(new Train(aMap));
}


