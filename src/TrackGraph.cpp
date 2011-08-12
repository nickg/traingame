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
#include "IterateTrack.hpp"

#include <stdexcept>

class TrackGraph : public ITrackGraph {
public:
   TrackGraph(IMapPtr map);

   // ITrackGraph interface
   void write_dot_file(const string& file) const;
   const graph::Node& root() const;
   const graph::Node& node(unsigned n) const;

private:
   void walk_track(TrackIterator it,
                   set<ITrackSegmentPtr>& visited,
                   graph::Node& where);
   graph::Node& add_node(graph::NodeType type);
   
   vector<graph::Node> nodes;
};

TrackGraph::TrackGraph(IMapPtr map)
{
   track::Position p;
   track::Direction d;
   tie(p, d) = map->start();

   graph::Node& root = add_node(graph::NODE_ROOT);

   set<ITrackSegmentPtr> visited;
   walk_track(iterate_track(map, p, d), visited, root);
}

void TrackGraph::walk_track(TrackIterator it,
                            set<ITrackSegmentPtr>& visited,
                            graph::Node& where)
{
   if (visited.find(it.track) != visited.end()) {
      // TODO: connect up with arc         
   }

   visited.insert(it.track);

   switch (it.status) {
   case TRACK_STATION:
      // TODO
   case TRACK_OK:
      walk_track(it.next(), visited, where);
      break;

   case TRACK_NO_MORE:
      break;

   case TRACK_CHOICE:
      throw runtime_error("can't graph track with choice yet!");
      
   }
}

graph::Node& TrackGraph::add_node(graph::NodeType type)
{
   unsigned id = nodes.size();
   graph::Node n = { id, type };
   nodes.push_back(n);
   return nodes.at(id);
}
   
const graph::Node& TrackGraph::root() const
{
   assert(nodes.size() > 0);
   return nodes.at(0);
}

const graph::Node& TrackGraph::node(unsigned n) const
{
   assert(n < nodes.size());
   return nodes.at(n);
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
