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

#ifndef INC_ITRAIN_HPP
#define INC_ITRAIN_HPP

#include "Platform.hpp"
#include "IRollingStock.hpp"
#include "IMap.hpp"
#include "ITrackSegment.hpp"

// Interface to managing complete trains
struct ITrain {
   virtual ~ITrain() {}

   virtual void render() const = 0;
   virtual void update(int a_delta) = 0;

   // Return a vector of the absolute position of the front of
   // the train
   virtual VectorF front() const = 0;

   // Return the track segment occupied by the front of the train
   virtual ITrackSegmentPtr track_segment() const = 0;

   // Return the tile occupied by the front of the train
   virtual track::Position tile() const = 0;

   // Return the speed of the train
   virtual double speed() const = 0;

   // Return the track direction of the front of the train
   virtual track::Direction direction() const = 0;

   // Return the controller for whatever's driving this train
   virtual IControllerPtr controller() = 0;
};

typedef shared_ptr<ITrain> ITrainPtr;

ITrainPtr make_train(IMapPtr a_map);

#endif
