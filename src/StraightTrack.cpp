//
//  Copyright (C) 2009-2010  Nick Gasson
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
#include "ILogger.hpp"
#include "XMLBuilder.hpp"
#include "Matrix.hpp"
#include "OpenGLHelper.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/lexical_cast.hpp>

using namespace placeholders;
using namespace boost;
using namespace track;

// Concrete implementation of straight-line pieces of track
class StraightTrack : public ITrackSegment,
                      public enable_shared_from_this<StraightTrack>,
                      private StraightTrackHelper,
                      private SleeperHelper {
public:
   StraightTrack(const Direction& a_direction);
   ~StraightTrack();
   
   void render() const {}
   void merge(IMeshBufferPtr buf) const;
   
   void set_origin(int x, int y, float h);
   float segment_length(const track::TravelToken& token) const { return 1.0f; }

   Connection next_position(const track::TravelToken& a_direction) const;
   bool is_valid_direction(const Direction& a_direction) const;
   void get_endpoints(vector<Point<int> >& a_list) const;
   void get_covers(vector<Point<int> >& output) const { }
   
   ITrackSegmentPtr merge_exit(Point<int> where, track::Direction dir);
   track::TravelToken get_travel_token(track::Position a_position,
      track::Direction a_direction) const;

   bool has_multiple_states() const { return false; }
   void next_state() {}
   void prev_state() {}
   void set_state_render_hint() {}

   // IXMLSerialisable interface
   xml::element to_xml() const;
   
private:
   void transform(const track::TravelToken& a_token, float delta) const;
   void ensure_valid_direction(const track::Direction& a_direction) const;
   
   Point<int> origin;  // Absolute position
   Direction direction;
   float height;
};

StraightTrack::StraightTrack(const Direction& a_direction)
   : direction(a_direction), height(0.0f)
{
   
}

StraightTrack::~StraightTrack()
{
   
}

void StraightTrack::set_origin(int x, int y, float h)
{
   origin = make_point(x, y);
   height = h;
}

track::TravelToken
StraightTrack::get_travel_token(track::Position a_position,
   track::Direction a_direction) const
{
   ensure_valid_direction(a_direction);

   track::TravelToken tok = {
      a_direction,
      a_position,
      bind(&StraightTrack::transform, this, _1, _2),
      track::flat_gradient_func,
      1
   };
   return tok;
}

void StraightTrack::transform(const track::TravelToken& a_token,
   float delta) const
{
   assert(delta < 1.0);

   if (a_token.direction == -direction)
      delta = 1.0 - delta;

   const float x_trans = direction == axis::X ? delta : 0;
   const float y_trans = direction == axis::Y ? delta : 0;

   glTranslated(static_cast<double>(origin.x) + x_trans,
      height,
      static_cast<double>(origin.y) + y_trans);

   if (direction == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (a_token.direction == -direction)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

ITrackSegmentPtr StraightTrack::merge_exit(Point<int> where,
   track::Direction dir)
{
#if 1
   debug() << "merge_exit where=" << where
           << " dir=" << dir
           << " me=" << origin
           << " mydir=" << direction;
#endif

   // See if we can make this a crossover track
   if (direction != dir && where == origin)
      return make_crossover_track();

   // See if we can make some points
   if (is_valid_direction(dir)) {
      // X-aligned points
      if (where == origin + make_point(-2, 1))
         return make_points(-axis::X, true);
      else if (where == origin + make_point(-2, -1))
         return make_points(-axis::X, false);
      else if (where == origin + make_point(2, 1))
         return make_points(axis::X, false);
      else if (where == origin + make_point(2, -1))
         return make_points(axis::X, true);

      // Y-aligned points
      if (where == origin + make_point(1, -2))
         return make_points(-axis::Y, false);
      else if (where == origin + make_point(-1, -2))
         return make_points(-axis::Y, true);
      else if (where == origin + make_point(1, 2))
         return make_points(axis::Y, true);
      else if (where == origin + make_point(-1, 2))
         return make_points(axis::Y, false);
   }
   
   // Not possible to merge
   return ITrackSegmentPtr();
}

bool StraightTrack::is_valid_direction(const Direction& a_direction) const
{
   if (direction == axis::X)
      return a_direction == axis::X || -a_direction == axis::X;
   else
      return a_direction == axis::Y || -a_direction == axis::Y;
}

void StraightTrack::get_endpoints(vector<Point<int> >& a_list) const
{
   a_list.push_back(origin);
}

void StraightTrack::ensure_valid_direction(const Direction& a_direction) const
{
   if (!is_valid_direction(a_direction))
      throw runtime_error
         ("Invalid direction on straight track: "
            + lexical_cast<string>(a_direction)
            + " (should be parallel to "
            + lexical_cast<string>(direction) + ")");
}

Connection StraightTrack::next_position(const track::TravelToken& a_token) const
{
   ensure_valid_direction(a_token.direction);

   if (a_token.direction == axis::X)
      return make_pair(make_point(origin.x + 1, origin.y), axis::X);
   else if (a_token.direction == -axis::X)
      return make_pair(make_point(origin.x - 1, origin.y), -axis::X);
   else if (a_token.direction == axis::Y)
      return make_pair(make_point(origin.x, origin.y + 1), axis::Y);
   else if (a_token.direction == -axis::Y)
      return make_pair(make_point(origin.x, origin.y - 1), -axis::Y);
   else
      assert(false);
}

void StraightTrack::merge(IMeshBufferPtr buf) const
{
   Vector<float> off = make_vector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   float y_angle = direction == axis::X ? 90.0f : 0.0f;

   merge_straight_rail(buf, off, y_angle);

   y_angle += 90.0f;

   off += rotate(make_vector(-0.4f, 0.0f, 0.0f), y_angle, 0.0f, 1.0f, 0.0f);
   
   for (int i = 0; i < 4; i++) {
      merge_sleeper(buf, off, y_angle);

      off += rotate(make_vector(0.25f, 0.0f, 0.0f), y_angle, 0.0f, 1.0f, 0.0f);
   }  
}

xml::element StraightTrack::to_xml() const
{
   return xml::element("straight_track")
      .add_attribute("align", direction == axis::X ? "x" : "y");
}

ITrackSegmentPtr make_straight_track(const Direction& a_direction)
{
   Direction real_dir(a_direction);
   
   // Direction must either be along axis::X or axis::Y but we
   // allow the opositite direction here too
   if (real_dir == -axis::X || real_dir == -axis::Y)
      real_dir = -real_dir;

   if (real_dir != axis::X && real_dir != axis::Y)
      throw runtime_error("Illegal straight track direction: "
         + lexical_cast<string>(a_direction));
   
   return ITrackSegmentPtr(new StraightTrack(real_dir));
}
