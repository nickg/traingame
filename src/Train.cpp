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
#include "TrackCommon.hpp"

#include <stdexcept>

#include <GL/gl.h>

using namespace std;
using namespace std::tr1;

// Concrete implementation of trains
class Train : public ITrain {
public:
   Train(IMapPtr aMap);
   
   void render() const;
   void update();

   Vector<float> front() const;
   
   double speed() const { return myEngine->speed(); }
   IControllerPtr controller() { return myEngine->controller(); }
private:
   IRollingStockPtr myEngine;
   IMapPtr myMap;

   // The length of a track segment can be found by calling segmentLength()
   // This delta value ranges from 0 to that length and indicates how far
   // along the segment the train is
   ITrackSegmentPtr mySegment;
   double mySegmentDelta;
   ITrackSegment::TransformFunc myTransformer;

   // Direction train is travelling along the track
   Vector<int> myDirection;

   // This updates the above two values
   void enterSegment(const Track::Connection& aConnection);

   static const double MODEL_YOFF;
};

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
   myEngine->update();

   // How many metres does a tile correspond to?
   const double M_PER_UNIT = 10.0;
   
   const double FPS = 30.0;
   mySegmentDelta += myEngine->speed() / FPS / M_PER_UNIT;
   
   if (mySegmentDelta >= mySegment->segmentLength()) {
      // Moved onto a new piece of track
      enterSegment(mySegment->nextPosition(myDirection));
   }
}

// Called when the train enters a new segment
// Resets the delta and gets the length of the new segment
void Train::enterSegment(const Track::Connection& aConnection)
{
   Point<int> pos;
   tie(pos, myDirection) = aConnection;
   
   debug() << "Train entered segment at " << pos
           << " moving " << myDirection;

   if (!myMap->isValidTrack(pos))
      throw runtime_error("Train fell off end of track!");

   mySegmentDelta = 0.0;
   mySegment = myMap->trackAt(pos);
   myTransformer = mySegment->transformFunc(myDirection);
}

void Train::render() const
{
   glPushMatrix();

   myTransformer(mySegmentDelta);
   glTranslatef(0.0f, track::RAIL_HEIGHT, 0.0f);
   myEngine->render();

   glPopMatrix();
}

Vector<float> Train::front() const
{
   // Call the transformer to compute the world location of
   // the front of the train
   glPushMatrix();
   glLoadIdentity();

   myTransformer(mySegmentDelta);

   float matrix[16];
   glGetFloatv(GL_MODELVIEW_MATRIX, matrix);
   
   glPopMatrix();

   return makeVector(matrix[12], matrix[13], matrix[14]);
}

// Make an empty train
ITrainPtr makeTrain(IMapPtr aMap)
{
   return ITrainPtr(new Train(aMap));
}


