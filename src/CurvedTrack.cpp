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

#include <cmath>
#include <cassert>
#include <stdexcept>
#include <set>

#include <GL/gl.h>
#include <boost/lexical_cast.hpp>
 
using namespace placeholders;
using namespace track;
using namespace boost;

// Concrete implementation of curved pieces of track
class CurvedTrack : public ITrackSegment,
                    public enable_shared_from_this<CurvedTrack>,
                    private CurvedTrackHelper {
public:
   CurvedTrack(track::Angle a_start_angle, track::Angle a_finish_angle,
      int a_radius);
   ~CurvedTrack();

   void render() const {}
   void merge(IMeshBufferPtr buf) const;

   void set_origin(int x, int y, float h);
   float segment_length(const track::TravelToken& a_token) const;

   Connection next_position(const track::TravelToken& a_token) const;
   bool is_valid_direction(const Direction& a_direction) const;
   void get_endpoints(vector<Point<int> >& a_list) const;
   void get_covers(vector<Point<int> >& output) const;
   
   ITrackSegmentPtr merge_exit(Point<int> where, track::Direction dir);

   xml::element to_xml() const;
   track::TravelToken get_travel_token(track::Position a_position,
      track::Direction a_direction) const;
    
   bool has_multiple_states() const { return false; }
   void next_state() {}
   void prev_state() {}
   void set_state_render_hint() {}
      
private:
   void transform(const track::TravelToken& a_token, float delta) const;
   void glTransform_toOrigin() const;
   Vector<int> cw_entry_vector() const;
   Vector<int> ccw_entry_vector() const;
   void ensure_valid_direction(const Direction& a_direction) const;

   Point<int> origin;
   int base_radius;
   track::Angle start_angle, finish_angle;
   float height;
};

CurvedTrack::CurvedTrack(track::Angle a_start_angle,
   track::Angle a_finish_angle,
   int a_radius)
   : origin(make_point(0, 0)), base_radius(a_radius),
     start_angle(a_start_angle), finish_angle(a_finish_angle),
     height(0.0f)
{
   
}

CurvedTrack::~CurvedTrack()
{

}

void CurvedTrack::set_origin(int x, int y, float h)
{
   origin = make_point(x, y);
   height = h;
}

track::TravelToken
CurvedTrack::get_travel_token(track::Position a_position,
   track::Direction a_direction) const
{
   ensure_valid_direction(a_direction);

   track::TravelToken tok = {
      a_direction,
      a_position,
      bind(&CurvedTrack::transform, this, _1, _2),
      track::flat_gradient_func,
      1
   };
   return tok;
}

void CurvedTrack::glTransform_toOrigin() const
{
   glTranslatef((base_radius-1)*-sin(deg_to_rad(start_angle)) - 0.5f, 0.0f,
      (base_radius-1)*-cos(deg_to_rad(start_angle)) - 0.5f);

   // There *must* be a way to incorporate this in the above translation
   // as a neat formula, but I really can't think of it
   // This is a complete a hack, but whatever...
   if (start_angle >= 90 && start_angle <= 180)
      glTranslatef(0.0f, 0.0f, 1.0f);
   
   if (start_angle >= 180 && start_angle <= 270)
      glTranslatef(1.0f, 0.0f, 0.0f);
}

void CurvedTrack::transform(const track::TravelToken& a_token, float delta) const
{
   assert(delta < segment_length(a_token));
   
   glTranslated(static_cast<double>(origin.x),
      height,
      static_cast<double>(origin.y));

   glTransform_toOrigin();

   bool backwards = a_token.direction == cw_entry_vector();
   
   double ratio = delta / segment_length(a_token);
   if (backwards)
      ratio = 1.0 - ratio;
      
   double angle = start_angle + (90.0 * ratio);

   glRotated(angle, 0.0, 1.0, 0.0);
   glTranslated(0.0, 0.0, static_cast<double>(base_radius - 0.5));

   if (backwards)
      glRotatef(180.0, 0, 1, 0);
}

float CurvedTrack::segment_length(const track::TravelToken& a_token) const
{
   // Assume curve is only through 90 degrees
   return M_PI * (static_cast<float>(base_radius) - 0.5f) / 2.0f;
}

//
// Imagine the train is travelling in a circle like this:
//
// (0, 0)
//              180
//             [-1 0]
//        <-------------^
//        |             |
//        |             |
// [0 1]  |             | [0 -1] 90
// 270    |             |
//        |             |
//        V------------>|
//             [1 0]
//               0
//
// Above are the vectors for /counter/-clockwise movement
//

// The vector the train is moving on if it enters clockwise
Vector<int> CurvedTrack::cw_entry_vector() const
{
   return make_vector<int>(-cos(deg_to_rad(finish_angle)), 0,
      sin(deg_to_rad(finish_angle)));
}

// The vector the train is moving on if it enters counter-clockwise
Vector<int> CurvedTrack::ccw_entry_vector() const
{
   return make_vector<int>(cos(deg_to_rad(start_angle)), 0.0,
      -sin(deg_to_rad(start_angle)));
}

void CurvedTrack::ensure_valid_direction(const Direction& a_direction) const
{
   if (!is_valid_direction(a_direction))
      throw runtime_error
         ("Invalid direction on curved track from "
            + lexical_cast<string>(start_angle) + " to "
            + lexical_cast<string>(finish_angle) + " degrees: "
            + lexical_cast<string>(a_direction)
            + " (should be "
            + lexical_cast<string>(cw_entry_vector()) + " or "
            + lexical_cast<string>(ccw_entry_vector()) + ")");
}

bool CurvedTrack::is_valid_direction(const Direction& a_direction) const
{
   return a_direction == cw_entry_vector() || a_direction == ccw_entry_vector();
}

Connection CurvedTrack::next_position(const track::TravelToken& a_token) const
{
   bool backwards;
   Vector<int> next_dir;
   if (a_token.direction == cw_entry_vector()) {
      next_dir = -ccw_entry_vector();
      backwards = true;
   }
   else if (a_token.direction == ccw_entry_vector()) {
      next_dir = -cw_entry_vector();
      backwards = false;
   }
   else
      assert(false);

   // Assuming 90 degree curves again
   const int cos_end = static_cast<int>(cos(deg_to_rad(finish_angle)));
   const int cos_start = static_cast<int>(cos(deg_to_rad(start_angle)));
   const int sin_end = static_cast<int>(sin(deg_to_rad(finish_angle)));
   const int sin_start = static_cast<int>(sin(deg_to_rad(start_angle)));

   int x_delta, y_delta;

   if (backwards)
      x_delta = y_delta = 0;
   else {
      x_delta = (base_radius - 1) * (sin_end - sin_start);
      y_delta = (base_radius - 1) * (cos_end - cos_start);
   }
   
   return make_pair(make_point(origin.x + x_delta + next_dir.x,
         origin.y + y_delta + next_dir.z),
      next_dir);
}

void CurvedTrack::get_endpoints(vector<Point<int> >& a_list) const
{
   a_list.push_back(origin);

   // Assuming 90 degree curves again
   const int cos_end = static_cast<int>(cos(deg_to_rad(finish_angle)));
   const int cos_start = static_cast<int>(cos(deg_to_rad(start_angle)));
   const int sin_end = static_cast<int>(sin(deg_to_rad(finish_angle)));
   const int sin_start = static_cast<int>(sin(deg_to_rad(start_angle)));
   
   const int x_delta = (base_radius - 1) * (sin_end - sin_start);
   const int y_delta = (base_radius - 1) * (cos_end - cos_start);
   
   a_list.push_back(make_point(origin.x + x_delta, origin.y + y_delta));
}

void CurvedTrack::get_covers(vector<Point<int> >& output) const
{
   vector<Point<int> > exits;
   get_endpoints(exits);

   const Point<int>& start = exits.at(0);
   const Point<int>& finish = exits.at(1);
   
   Point<int> true_origin =
      (start_angle == 90 || start_angle == 270)
      ? make_point(finish.x, start.y)
      : make_point(start.x, finish.y);
   
   set<Point<int> > tmp;

   // A fiddle factor to put the cover tiles in the best location
   const float fiddle_radius = static_cast<float>(base_radius) - 0.5f;

   const float sign = (start_angle == 0 || start_angle == 180) ? 1.0f : -1.0f;

   for (track::Angle angle = start_angle; angle < finish_angle; angle += 5) {
      float x = fiddle_radius * sign * cos(deg_to_rad(angle));
      float y = fiddle_radius * sign * sin(deg_to_rad(angle));
      Point<int> p = make_point(static_cast<int>(x),
         static_cast<int>(y));

      if (abs(p.x) >= base_radius || abs(p.y) >= base_radius)
         continue;
      
      Point<int> actual = p + true_origin;

      if (actual != start && actual != finish)
         tmp.insert(actual);
   }

   copy(tmp.begin(), tmp.end(), back_inserter(output));
}

ITrackSegmentPtr CurvedTrack::merge_exit(Point<int> where, track::Direction dir)
{
   // See if this is already an exit
   if (is_valid_direction(dir)) {
      vector<Point<int> > exits;
      get_endpoints(exits);

      for (vector<Point<int> >::iterator it = exits.begin();
           it != exits.end(); ++it)
         if (*it == where)
            return shared_from_this();
   }

   // No way to merge this as an exit
   return ITrackSegmentPtr();
}

void CurvedTrack::merge(IMeshBufferPtr buf) const
{
   const Vector<float> off = make_vector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   merge_curved_track(buf, off, base_radius, start_angle, finish_angle);
}

xml::element CurvedTrack::to_xml() const
{
   return xml::element("curved-track")
      .add_attribute("start-angle", start_angle)
      .add_attribute("finish-angle", finish_angle)
      .add_attribute("radius", base_radius);
}

ITrackSegmentPtr make_curved_track(track::Angle a_start_angle,
   track::Angle a_finish_angle, int a_radius)
{
   assert(a_start_angle < a_finish_angle);
   
   return ITrackSegmentPtr
      (new CurvedTrack(a_start_angle, a_finish_angle, a_radius));
}
