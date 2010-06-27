//
//  Copyright (C) 2010  Nick Gasson
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

#include "ITrackSegment.hpp"
#include "TrackCommon.hpp"
#include "XMLBuilder.hpp"

#include <stdexcept>

// A generic track implementation based on Bezier curves
class GenTrack : public ITrackSegment,
                 private SleeperHelper,
                 private BezierHelper {
public:
   GenTrack(Vector<int> delta,
            track::Direction entry_dir,
            track::Direction exit_dir);

   // ITrackSegment interface
   void render() const {}
   void merge(IMeshBufferPtr buf) const;
   void set_origin(int x, int y, float h);
   float segment_length(const track::TravelToken& token) const;
   bool is_valid_direction(const track::Direction& dir) const;
   track::Connection next_position(const track::TravelToken& token) const;
   void get_endpoints(vector<Point<int> >& output) const;
   void get_covers(vector<Point<int> >& output) const;
   ITrackSegmentPtr merge_exit(Point<int> where, track::Direction dir);
   track::TravelToken get_travel_token(track::Position pos,
      track::Direction dir) const;
   void next_state() {}
   void prev_state() {}
   bool has_multiple_states() const { return false; }
   void set_state_render_hint() {}

   // IXMLSerialisable interface
   xml::element to_xml() const;
   
private:

};

GenTrack::GenTrack(Vector<int> delta,
                   track::Direction entry_dir,
                   track::Direction exit_dir)
{

}

void GenTrack::merge(IMeshBufferPtr buf) const
{

}

void GenTrack::set_origin(int x, int y, float h)
{

}

float GenTrack::segment_length(const track::TravelToken& token) const
{
   return 1.0f;
}

bool GenTrack::is_valid_direction(const track::Direction& dir) const
{
   return false;
}

track::Connection GenTrack::next_position(const track::TravelToken& token) const
{
   throw runtime_error("next_position not implemented");
}

void GenTrack::get_endpoints(vector<Point<int> >& output) const
{

}

void GenTrack::get_covers(vector<Point<int> >& output) const
{

}

ITrackSegmentPtr GenTrack::merge_exit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

track::TravelToken GenTrack::get_travel_token(track::Position pos,
                                              track::Direction dir) const
{
   throw runtime_error("get_travel_token not implemented");
}

xml::element GenTrack::to_xml() const
{
   return xml::element("gen-track");
}

ITrackSegmentPtr make_gen_track(Vector<int> delta,
                                track::Direction entry_dir,
                                track::Direction exit_dir)
{
   return ITrackSegmentPtr(new GenTrack(delta, entry_dir, exit_dir));
}
