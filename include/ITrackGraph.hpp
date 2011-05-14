//
//  Copyright (C) 2011  Nick Gasson
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

#ifndef INC_ITRACK_GRAPH_HPP
#define INC_ITRACK_GRAPH_HPP

#include "IMap.hpp"

struct ITrackGraph {
   virtual ~ITrackGraph() {}

   // Dump the track graph in DOT format for rendering with Graphviz
   virtual void write_dot_file(const string& file) const = 0;
};

typedef shared_ptr<ITrackGraph> ITrackGraphPtr;

ITrackGraphPtr make_track_graph(IMapPtr map);

#endif  // INC_ITRACK_GRAPH_HPP
