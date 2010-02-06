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

#include "IterateTrack.hpp"

// Find the next piece of track
TrackIterator TrackIterator::next() const
{
   if (status == TRACK_NO_MORE)
      return *this;
   
   track::Direction dir;
   track::Position pos;
   tie(pos, dir) = track->nextPosition(token);

   return iterateTrack(map, pos, dir);   
}

// Build an iterator object for a given track segment
TrackIterator iterateTrack(IMapPtr aMap, track::Position aPosition,
                           track::Direction aDirection)
{
   TrackIterator it;
   it.map = aMap;
   it.status = TRACK_OK ;
   
   if (aMap->isValidTrack(aPosition)) {
      it.track = aMap->trackAt(aPosition);
      it.token = it.track->getTravelToken(aPosition, aDirection);
   }
   else {
      // Fell off the end
      it.track = ITrackSegmentPtr();
      it.status = TRACK_NO_MORE;
      return it;
   }

   // Are we sitting on a station?
   typedef list<Point<int> > PointList;
   PointList endpoints;
   it.track->getEndpoints(endpoints);

   IStationPtr station;
   for (PointList::const_iterator p = endpoints.begin();
        p != endpoints.end(); ++p)
      if ((it.station = aMap->stationAt(makePoint((*p).x, (*p).y)))) {
         it.status = TRACK_STATION;
         break;
      }

   if (it.track->hasMultipleStates())
       it.status = TRACK_CHOICE;
   
   return it;   
}
