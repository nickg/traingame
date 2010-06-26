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

#include <cassert>

// Find the next piece of track
TrackIterator TrackIterator::next() const
{
   if (status == TRACK_NO_MORE)
      return *this;
   
   track::Direction dir;
   track::Position pos;
   tie(pos, dir) = track->next_position(token);

   return iterate_track(map, pos, dir);   
}

// Build an iterator object for a given track segment
TrackIterator iterate_track(IMapPtr a_map, track::Position a_position,
                           track::Direction a_direction)
{
   TrackIterator it;
   it.map = a_map;
   it.status = TRACK_OK ;
   
   if (a_map->is_valid_track(a_position)) {
      it.track = a_map->track_at(a_position);
      it.token = it.track->get_travel_token(a_position, a_direction);
   }
   else {
      // Fell off the end
      it.track = ITrackSegmentPtr();
      it.status = TRACK_NO_MORE;
      return it;
   }

   // Are we sitting on a station?
   typedef vector<Point<int> > PointList;
   PointList endpoints;
   it.track->get_endpoints(endpoints);

   IStationPtr station;
   for (PointList::const_iterator p = endpoints.begin();
        p != endpoints.end(); ++p)
      if ((it.station = a_map->station_at(make_point((*p).x, (*p).y)))) {
         it.status = TRACK_STATION;
         break;
      }

   if (it.token.num_exits > 1) {
       assert(it.track->has_multiple_states());
       it.status = TRACK_CHOICE;
   }
   else
       assert(it.token.num_exits == 1);
   
   return it;   
}
