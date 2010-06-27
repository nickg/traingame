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
#include "IXMLSerialisable.hpp"
#include "XMLBuilder.hpp"
#include "BezierCurve.hpp"
#include "IMesh.hpp"
#include "TrackCommon.hpp"
#include "OpenGLHelper.hpp"
#include "ILogger.hpp"
#include "Matrix.hpp"

#include <cassert>
#include <stdexcept>

#include <boost/cast.hpp>

// Like StraightTrack but with a change of height
class SlopeTrack : public ITrackSegment,
                   private SleeperHelper,
                   private BezierHelper {
public:
   SlopeTrack(track::Direction axis, Vector<float> slope,
      Vector<float> slope_before, Vector<float> slope_after);

   // ITrackSegment interface
   void render() const {}
   void merge(IMeshBufferPtr buf) const;
   void set_origin(int x, int y, float h);
   float segment_length(const track::TravelToken& token) const;
   track::TravelToken get_travel_token(track::Position pos,
      track::Direction dir) const;
   bool is_valid_direction(const track::Direction& dir) const;
   track::Connection next_position(const track::TravelToken& token) const;
   void get_endpoints(vector<Point<int> >& output) const;
   void get_covers(vector<Point<int> >& output) const {};
   ITrackSegmentPtr merge_exit(Point<int> where, track::Direction dir);
   
   bool has_multiple_states() const { return false; }
   void next_state() {}
   void prev_state() {}
   void set_state_renderHint() {}

   // IXMLSerialisable inteface
   xml::element to_xml() const;

private:
   void ensure_valid_direction(const track::Direction& dir) const;
   void transform(const track::TravelToken& token, float delta) const;
   float gradient(const track::TravelToken& token, float delta) const;
   
   Point<int> origin;
   float height;
   IMeshBufferPtr rail_buf;
   track::Direction axis;
   float length, y_offset;
   BezierCurve<float> curve;
};

SlopeTrack::SlopeTrack(track::Direction axis, Vector<float> slope,
   Vector<float> slope_before, Vector<float> slope_after)
   : height(0.0f), axis(axis), y_offset(0.0f)
{
   const float OFF = 0.1f;

   assert(axis == axis::X || axis == axis::Y);
   
   const Vector<float> avg_before = (slope + slope_before) / 2.0f;
   const Vector<float> avg_after = (slope + slope_after) / 2.0f;

   if (slope.y < 0.0f)
      y_offset = abs(slope.y);

   const float h_factor0 = sqrt(OFF / (1 + avg_before.y * avg_before.y));
   const float h_factor1 = sqrt(OFF / (1 + avg_after.y * avg_after.y));

   const float x_delta0 = h_factor0;
   const float y_delta0 = h_factor0 * avg_before.y;

   const float x_delta1 = h_factor1;
   const float y_delta1 = h_factor1 * avg_after.y;
   
   Vector<float> p1 = make_vector(0.0f, 0.0f, 0.0f);
   Vector<float> p2 = make_vector(x_delta0, y_delta0, 0.0f);
   Vector<float> p3 = make_vector(1.0f - x_delta1, slope.y - y_delta1, 0.0f);
   Vector<float> p4 = make_vector(1.0f, slope.y, 0.0f);

   curve = make_bezier_curve(p1, p2, p3, p4);
   length = curve.length;

   rail_buf = make_bezier_railMesh(curve);
}

void SlopeTrack::merge(IMeshBufferPtr buf) const
{
   Vector<float> off = make_vector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   float y_angle = axis == axis::Y ? -90.0f : 0.0f;

   off += rotateY(make_vector(-0.5f, 0.0f, 0.0f), y_angle);
   
   buf->merge(rail_buf, off, y_angle);
   
   // Draw the sleepers
   for (float t = 0.1f; t < 1.0f; t += 0.25f) {
      const Vector<float> curve_value = curve(t);

#if 0
      // Should the sleepers be at the same angle as the slope?
      const Vector<float> deriv = curve.deriv(t);
      const float angle =
         rad_to_deg<float>(atanf(deriv.y / deriv.x));
#endif

      Vector<float> t = make_vector(curve_value.x, curve_value.y, 0.0f);

      merge_sleeper(buf, off + rotateY(t, y_angle), y_angle);
   }
}

void SlopeTrack::set_origin(int x, int y, float h)
{
   origin = make_point(x, y);
   height = h + y_offset;
}

float SlopeTrack::segment_length(const track::TravelToken& token) const
{
   return length;
}

bool SlopeTrack::is_valid_direction(const track::Direction& dir) const
{
   if (axis == axis::X)
      return dir == axis::X || -dir == axis::X;
   else
      return dir == axis::Y || -dir == axis::Y;
}

void SlopeTrack::ensure_valid_direction(const track::Direction& dir) const
{
   if (!is_valid_direction(dir))
      throw runtime_error
         ("Invalid direction on straight track: "
            + boost::lexical_cast<string>(dir)
            + " (should be parallel to "
            + boost::lexical_cast<string>(axis) + ")");
}

track::Connection SlopeTrack::next_position(
   const track::TravelToken& token) const
{
   ensure_valid_direction(token.direction);

   if (token.direction == axis::X)
      return make_pair(make_point(origin.x + 1, origin.y), axis::X);
   else if (token.direction == -axis::X)
      return make_pair(make_point(origin.x - 1, origin.y), -axis::X);
   else if (token.direction == axis::Y)
      return make_pair(make_point(origin.x, origin.y + 1), axis::Y);
   else if (token.direction == -axis::Y)
      return make_pair(make_point(origin.x, origin.y - 1), -axis::Y);
   else
      assert(false);
}

track::TravelToken SlopeTrack::get_travel_token(track::Position pos,
      track::Direction dir) const
{
   using namespace placeholders;
   
   ensure_valid_direction(dir);

   track::TravelToken tok = {
      dir,
      pos,
      bind(&SlopeTrack::transform, this, _1, _2),
      bind(&SlopeTrack::gradient, this, _1, _2),
      1
   };
   return tok;
}

float SlopeTrack::gradient(const track::TravelToken& token, float delta) const
{
   assert(delta < length && delta >= 0.0f);
   
   if (token.direction == -axis)
      delta = length - delta;

   return curve.deriv(delta / length).y;
}

void SlopeTrack::transform(const track::TravelToken& token, float delta) const
{
   assert(delta < length && delta >= 0.0f);

#if 0
   debug() << "f(0)=" << curve(0.0f)
           << " f(0.5)=" << curve(0.5f)
           << " f(1.0)=" << curve(1.0f);

   debug() << "f'(0)=" << curve.deriv(0.0f)
           << " f'(0.5)=" << curve.deriv(0.5f)
           << " f'(1.0)=" << curve.deriv(1.0f);
#endif
   
   if (token.direction == -axis)
      delta = length - delta;

   const float curve_delta = delta / length;

   const Vector<float> curve_value = curve(curve_delta);
   
   const float x_trans = axis == axis::X ? curve_value.x : 0.0f;
   const float y_trans =curve_value.y;
   const float z_trans = axis == axis::Y ? curve_value.x : 0.0f;
   
   glTranslated(static_cast<double>(origin.x) + x_trans,
      height + y_trans,
      static_cast<double>(origin.y) + z_trans);

   if (axis == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (token.direction == -axis)
      glRotated(-180.0, 0.0, 1.0, 0.0);

   const Vector<float> deriv = curve.deriv(curve_delta);
   const float angle =
      rad_to_deg<float>(atanf(deriv.y / deriv.x));

   if (token.direction == -axis)
      glRotatef(-angle, 0.0f, 0.0f, 1.0f);
   else
      glRotatef(angle, 0.0f, 0.0f, 1.0f);
}

void SlopeTrack::get_endpoints(vector<Point<int> >& output) const
{
   output.push_back(origin);
}

ITrackSegmentPtr SlopeTrack::merge_exit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

xml::element SlopeTrack::to_xml() const
{
   return xml::element("slope_track")
      .add_attribute("align", axis == axis::X ? "x" : "y");
}

ITrackSegmentPtr make_slope_track(track::Direction axis, Vector<float> slope,
   Vector<float> slope_before, Vector<float> slope_after)
{
   return ITrackSegmentPtr(
      new SlopeTrack(axis, slope, slope_before, slope_after));
}
