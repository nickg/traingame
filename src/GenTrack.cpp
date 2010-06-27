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
   BezierCurve<float> curve;
   IMeshBufferPtr rail_buf;

   Point<int> origin;
   float height;
   Vector<int> delta;

   typedef tuple<Vector<int>,
                 track::Direction,
                 track::Direction> Parameters;
   typedef map<Parameters, IMeshBufferPtr> MeshCache; 
   static MeshCache mesh_cache;
};

GenTrack::MeshCache GenTrack::mesh_cache;

GenTrack::GenTrack(Vector<int> delta,
                   track::Direction entry_dir,
                   track::Direction exit_dir)
   : delta(delta)
{
   Vector<float> delta_f = make_vector(
      static_cast<float>(delta.x),
      0.0f,
      static_cast<float>(delta.y));

   Vector<float> entry_dir_norm = make_vector(
      static_cast<float>(entry_dir.x),
      0.0f,
      static_cast<float>(entry_dir.y)).normalise();
   
   Vector<float> exit_dir_norm = make_vector(
      static_cast<float>(exit_dir.x),
      0.0f,
      static_cast<float>(exit_dir.y)).normalise();

   float pinch_length = (delta_f.length() + 1.0f) / 3.0f;
   
   Vector<float> p1 = entry_dir_norm * -0.5f;
   Vector<float> p2 = entry_dir_norm * pinch_length;
   Vector<float> p3 = delta_f - (exit_dir_norm * pinch_length);
   Vector<float> p4 = delta_f + (exit_dir_norm * 0.5f);

   curve = make_bezier_curve(p1, p2, p3, p4);
   
   Parameters parms = make_tuple(delta, entry_dir, exit_dir);
   MeshCache::iterator it = mesh_cache.find(parms);
   if (it == mesh_cache.end()) {
      rail_buf = make_bezier_railMesh(curve);
      mesh_cache[parms] = rail_buf;
   }
   else
      rail_buf = (*it).second;
}

void GenTrack::merge(IMeshBufferPtr buf) const
{
   Vector<float> off = make_vector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   buf->merge(rail_buf, off, 0.0f);
}

void GenTrack::set_origin(int x, int y, float h)
{
   origin = make_point(x, y);
   height = h;
}

float GenTrack::segment_length(const track::TravelToken& token) const
{
   return curve.length;
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
   output.push_back(origin);

   if (delta.x > 0 || delta.y > 0)
      output.push_back(
         make_point(origin.x + delta.x - 0, origin.y + delta.y - 0));
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
