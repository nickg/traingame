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

#include "ITrackGraph.hpp"
#include "ILogger.hpp"

#include <stdexcept>

class TrackGraph : public ITrackGraph {
public:
   TrackGraph(IMapPtr map);

   void write_dot_file(const string& file) const;
};

TrackGraph::TrackGraph(IMapPtr map)
{
   
}

void TrackGraph::write_dot_file(const string& file) const
{
   ofstream of(file.c_str());
   if (!of.good())
      throw runtime_error("failed to open " + file);

   log() << "Wrote track graph to " << file;
}

ITrackGraphPtr make_track_graph(IMapPtr map)
{
   return ITrackGraphPtr(new TrackGraph(map));
}
