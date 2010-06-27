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
#include "ILogger.hpp"
#include "XMLBuilder.hpp"
#include "BezierCurve.hpp"
#include "OpenGLHelper.hpp"
#include "Matrix.hpp"

#include <cassert>
#include <map>

#include <boost/lexical_cast.hpp>

// Spline curves which start and finish in the same direction
class SBend : public ITrackSegment,
              private SleeperHelper,
              private BezierHelper {
public:
   SBend(track::Direction dir, int straight, int off);

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
   void transform(const track::TravelToken& token, float delta) const;
   void ensure_valid_direction(track::Direction dir) const;

   Point<int> origin;
   const int straight, offset;
   float height;
   track::Direction axis;

   BezierCurve<float> curve;
   IMeshBufferPtr rail_buf;

   typedef tuple<int, int> Parameters;
   typedef map<Parameters, IMeshBufferPtr> MeshCache; 
   static MeshCache mesh_cache;
};

SBend::MeshCache SBend::mesh_cache;

SBend::SBend(track::Direction dir, int straight, int off)
   : straight(straight), offset(off),
     height(0.0f),
     axis(dir)
{
   assert(straight > 0);

   const float pinch = static_cast<float>(straight) / 3.0f;

   const int reflect = (axis == axis::Y ? -1 : 1);
   
   const float offsetF = static_cast<float>(offset * reflect);
   const float straightF = static_cast<float>(straight);
   
   Vector<float> p1 = make_vector(0.0f, 0.0f, 0.0f);
   Vector<float> p2 = make_vector(pinch, 0.0f, 0.0f);
   Vector<float> p3 = make_vector(straightF - pinch, 0.0f, offsetF);
   Vector<float> p4 = make_vector(straightF, 0.0f, offsetF);

   curve = make_bezier_curve(p1, p2, p3, p4);

   Parameters parms = make_tuple(straight, offset * reflect);
   MeshCache::iterator it = mesh_cache.find(parms);
   if (it == mesh_cache.end()) {
      rail_buf = make_bezier_rail_mesh(curve);
      mesh_cache[parms] = rail_buf;
   }
   else
      rail_buf = (*it).second;
}

void SBend::set_origin(int x, int y, float h)
{
   origin = make_point(x, y);
   height = h;
}

void SBend::merge(IMeshBufferPtr buf) const
{
   Vector<float> off = make_vector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   float y_angle = axis == axis::Y ? -90.0f : 0.0f;

   {
      Vector<float> t = make_vector(-0.5f, 0.0f, 0.0f);
      buf->merge(rail_buf, off + rotateY(t, y_angle), y_angle);
   }

   // Draw the sleepers
   for (float i = 0.2f; i < curve.length; i += 0.25f) {
      Vector<float> v = curve(i / curve.length);

      Vector<float> t = make_vector(v.x - 0.5f, 0.0f, v.z);
      
      const Vector<float> deriv = curve.deriv(i / curve.length);
      const float angle =
         rad_to_deg<float>(atanf(deriv.z / deriv.x));

      merge_sleeper(buf, off + rotateY(t, y_angle), y_angle - angle);
   }
}

float SBend::segment_length(const track::TravelToken& token) const
{
   return curve.length;
}

bool SBend::is_valid_direction(const track::Direction& dir) const
{
   if (axis == axis::X)
      return dir == axis::X || -dir == axis::X;
   else
      return dir == axis::Y || -dir == axis::Y;
}

track::Connection SBend::next_position(const track::TravelToken& token) const
{
   ensure_valid_direction(token.direction);

   Point<int> disp;
   
   if (token.direction == axis::X)
      disp = make_point(straight, offset);
   else if (token.direction == -axis::X)
      disp = make_point(-1, 0);
   else if (token.direction == axis::Y)
      disp = make_point(offset, straight);
   else if (token.direction == -axis::Y)
      disp = make_point(0, -1);
   else
      assert(false);

   return make_pair(origin + disp, token.direction);
}

void SBend::get_endpoints(vector<Point<int> >& output) const
{
   output.push_back(origin);

   if (axis == axis::X)
      output.push_back(origin + make_point(straight - 1, offset));
   else
      output.push_back(origin + make_point(offset, straight - 1));
}

void SBend::get_covers(vector<Point<int> >& output) const
{
   vector<Point<int> > exits;
   get_endpoints(exits);

   set<Point<int> > tmp;
   
   for (float f = 0.0f; f < 1.0f; f += 0.1f) {
      Vector<float> curve_value = curve(f);
      
      curve_value.z += 0.5f;
      
      int x, y;
      if (axis == axis::X) {
         x = static_cast<int>(floor(curve_value.x + origin.x));
         y = static_cast<int>(floor(curve_value.z + origin.y));
      }
      else {
         x = -static_cast<int>(floor(curve_value.z - origin.x));
         y = static_cast<int>(floor(curve_value.x + origin.y));
      }

      Point<int> p = make_point(x, y);

      if (p != exits.at(0) && p != exits.at(1))
         tmp.insert(p);
   }

   copy(tmp.begin(), tmp.end(), back_inserter(output));
}

ITrackSegmentPtr SBend::merge_exit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

track::TravelToken SBend::get_travel_token(track::Position pos,
   track::Direction dir) const
{
   using namespace placeholders;
   
   ensure_valid_direction(dir);

   track::TravelToken tok = {
      dir,
      pos,
      bind(&SBend::transform, this, _1, _2),
      track::flat_gradient_func,
      1
   };
   return tok;
}

void SBend::transform(const track::TravelToken& token, float delta) const
{
   assert(delta < curve.length);

   const bool backwards = token.direction == -axis;
   if (backwards)
      delta = curve.length - delta;

   const float curve_delta = delta / curve.length;

   Vector<float> curve_value = curve(curve_delta);

   const Vector<float> deriv = curve.deriv(curve_delta);
   const float angle =
      rad_to_deg<float>(atanf(deriv.z / deriv.x));

   float x_trans, y_trans;
   if (axis == axis::X) {
      x_trans = curve_value.x;
      y_trans = curve_value.z;
   }
   else if (axis == axis::Y) {
      x_trans = -curve_value.z;
      y_trans = curve_value.x;
   }
   else
      assert(false);

   glTranslatef(
      static_cast<float>(origin.x) + x_trans,
      height,
      static_cast<float>(origin.y) + y_trans);
      
   if (axis == axis::Y)
      glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
      
   glTranslatef(-0.5f, 0.0f, 0.0f);

   glRotatef(-angle, 0.0f, 1.0f, 0.0f);

   if (backwards)
      glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
}

void SBend::ensure_valid_direction(track::Direction dir) const
{
   if (!is_valid_direction(dir))
      throw runtime_error
         ("Invalid direction on s-bend track: "
            + boost::lexical_cast<string>(dir)
            + " (should be parallel to "
            + boost::lexical_cast<string>(axis) + ")");
}

xml::element SBend::to_xml() const
{
   return xml::element("sbend-track")
      .add_attribute("align", axis == axis::X ? "x" : "y")
      .add_attribute("offset", offset)
      .add_attribute("straight", straight);
}

ITrackSegmentPtr makeSBend(track::Direction dir, int straight, int off)
{
   return ITrackSegmentPtr(new SBend(dir, straight, off));
}
