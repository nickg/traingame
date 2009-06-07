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

#ifndef INC_ITERATE_TRACK_HPP
#define INC_ITERATE_TRACK_HPP

#include "IMap.hpp"
#include "ITrackSegment.hpp"
#include "IStation.hpp"

//
// Some helper functions to make automated exploration of the
// track layout easier
//

// Communicates the status of the iteration
enum IterationStatus {
   TRACK_OK,          // No problems
   TRACK_NO_MORE,     // Run off the end of the track!
   TRACK_STATION,     // This segment contains a valid station
};

// Handle to the current iteration
struct TrackIterator {
   ITrackSegmentPtr track;
   IStationPtr station;
   IterationStatus status;
   track::Direction direction;
   IMapPtr map;

   TrackIterator next() const;
};

// Kick off the iteration at an initial track segment
TrackIterator iterateTrack(IMapPtr aMap, ITrackSegmentPtr aTrackSegment,
                           track::Direction aDirection);

#endif
