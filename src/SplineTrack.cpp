//
//  Copyright (C) 2010-2012  Nick Gasson
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
#include "OpenGLHelper.hpp"
#include "ILogger.hpp"
#include "Matrix.hpp"

#include <stdexcept>

// A generic track implementation based on Bezier curves
class SplineTrack : public ITrackSegment,
                    private SleeperHelper,
                    private BezierHelper {
public:
   SplineTrack(VectorI delta,
               track::Direction entry_dir,
               track::Direction exit_dir);

   // ITrackSegment interface
   void render() const;
   void merge(IMeshBufferPtr buf) const;
   void set_origin(int x, int y, float h);
   float segment_length(const track::TravelToken& token) const;
   bool is_valid_direction(const track::Direction& dir) const;
   track::Connection next_position(const track::TravelToken& token) const;
   void get_endpoints(PointList& output) const;
   void get_covers(PointList& output) const;
   void get_height_locked(PointList& output) const;
   ITrackSegmentPtr merge_exit(PointI where, track::Direction dir);
   track::TravelToken get_travel_token(track::Position pos,
                                       track::Direction dir) const;
   void next_state() {}
   void prev_state() {}
   bool has_multiple_states() const { return false; }
   void set_state_render_hint() {}

   // IXMLSerialisable interface
   xml::element to_xml() const;

private:
   typedef vector<Point<float> > Polygon;
   void bounding_polygon(Polygon& poly) const;
   static bool point_in_polygon(const Polygon& poly, Point<float> p);

   float extend_from_center(track::Direction dir) const;
   void ensure_valid_direction(track::Direction dir) const;
   void transform(const track::TravelToken& token,
                  float delta, bool backwards) const;
   float rotation_at(float delta) const;

   BezierCurve<float> curve;
   IMeshBufferPtr rail_buf;

   PointI origin;
   float height;
   VectorI delta;
   track::Direction entry_dir, exit_dir;
   Polygon bounds;

   typedef tuple<VectorI,
                 track::Direction,
                 track::Direction> Parameters;
   typedef map<Parameters, IMeshBufferPtr> MeshCache;
   static MeshCache mesh_cache;
};

SplineTrack::MeshCache SplineTrack::mesh_cache;

SplineTrack::SplineTrack(VectorI delta,
                         track::Direction entry_dir,
                         track::Direction exit_dir)
   : delta(delta),
     entry_dir(entry_dir),
     exit_dir(exit_dir)
{
   Vector<float> delta_f = make_vector(
      static_cast<float>(delta.x),
      0.0f,
      static_cast<float>(delta.y));

   Vector<float> entry_dir_norm = make_vector(
      static_cast<float>(entry_dir.x),
      0.0f,
      static_cast<float>(entry_dir.z)).normalise();

   Vector<float> exit_dir_norm = make_vector(
      static_cast<float>(exit_dir.x),
      0.0f,
      static_cast<float>(exit_dir.z)).normalise();

   float pinch_length = (delta_f.length() + 1.0f) / 3.0f;

   const float entry_extend = extend_from_center(entry_dir);
   const float exit_extend = extend_from_center(exit_dir);

   Vector<float> p1 = entry_dir_norm * -entry_extend;
   Vector<float> p2 = entry_dir_norm * (pinch_length - entry_extend);
   Vector<float> p3 = delta_f - (exit_dir_norm * (pinch_length - exit_extend));
   Vector<float> p4 = delta_f + (exit_dir_norm * exit_extend);

   curve = make_bezier_curve(p1, p2, p3, p4);

   Parameters parms = make_tuple(delta, entry_dir, exit_dir);
   MeshCache::iterator it = mesh_cache.find(parms);
   if (it == mesh_cache.end()) {
      rail_buf = make_bezier_rail_mesh(curve);
      mesh_cache[parms] = rail_buf;
   }
   else
      rail_buf = (*it).second;

   bounding_polygon(bounds);
}

float SplineTrack::extend_from_center(track::Direction dir) const
{
   // Track must extend from the centre to the edge of a tile
   const float x_sq = static_cast<float>(dir.x * dir.x);
   const float y_sq = static_cast<float>(dir.z * dir.z);

   if (abs(dir.x) == abs(dir.z))
      return sqrtf(2.0f) * 0.5f;
   else if (abs(dir.x) < abs(dir.z))
      return sqrtf(x_sq / y_sq + 1) * 0.5f;
   else
      return sqrtf(y_sq / x_sq + 1) * 0.5f;
}

void SplineTrack::merge(IMeshBufferPtr buf) const
{
   Vector<float> off = make_vector(
      static_cast<float>(origin.x),
      height,
      static_cast<float>(origin.y));

   buf->merge(rail_buf, off, 0.0f);

   // Draw the sleepers

   const float sleeper_sep = 0.25f;

   float delta = 0;
   int n = 0;
   do {
      ++n;
      delta = ((curve.length - sleeper_sep) / n) - sleeper_sep;
   } while (delta > sleeper_sep / n);

   for (int i = 0; i <= n; i++) {
      float pos = (sleeper_sep / 2) + i * (sleeper_sep + delta);

      float u_curve_delta;
      Vector<float> v = curve.linear(pos / curve.length, &u_curve_delta);

      const Vector<float> deriv = curve.deriv(u_curve_delta);
      const float angle =
         rad_to_deg<float>(atanf(deriv.z / deriv.x));

      merge_sleeper(buf, off + v, -angle);
   }
}

void SplineTrack::render() const
{
#if 0
   // Draw the bounding polygon
   glPushMatrix();

   glTranslatef(origin.x, 0.0f, origin.y);
   glColor3f(0.1f, 0.1f, 0.8f);

   glBegin(GL_LINE_LOOP);
   for (Polygon::const_iterator it = bounds.begin();
        it != bounds.end();
        ++it)
      glVertex3f((*it).x, 0.1f, (*it).y);
   glEnd();

   glPopMatrix();
#endif

#if 0
   // Draw control points
   glPushMatrix();
   glPushAttrib(GL_LINE_BIT);

   glTranslatef(origin.x, 0.0f, origin.y);
   glColor3f(0.8f, 0.1f, 0.1f);
   glLineWidth(4.0f);

   glBegin(GL_LINES);
   for (int i = 0; i < 4; i++)
      gl::vertex(curve.p[i] + make_vector(0.0f, 0.2f, 0.0f));
   glEnd();

   glPopAttrib();
   glPopMatrix();
#endif
}

void SplineTrack::set_origin(int x, int y, float h)
{
   origin = make_point(x, y);
   height = h;
}

float SplineTrack::segment_length(const track::TravelToken& token) const
{
   return curve.length;
}

bool SplineTrack::is_valid_direction(const track::Direction& dir) const
{
   return dir == entry_dir || dir == -exit_dir;
}

void SplineTrack::ensure_valid_direction(track::Direction dir) const
{
   if (!is_valid_direction(dir))
      throw runtime_error(
         "Invalid direction on gen-track: "
         + boost::lexical_cast<string>(dir)
         + " (should be "
         + boost::lexical_cast<string>(entry_dir)
         + " or " + boost::lexical_cast<string>(-exit_dir) + ")");
}

track::Connection
SplineTrack::next_position(const track::TravelToken& token) const
{
   ensure_valid_direction(token.direction);

   if (token.direction == entry_dir) {
      PointI off =
         make_point(exit_dir.x, exit_dir.z) + make_point(delta.x, delta.y);
      return make_pair(origin + off, exit_dir);
   }
   else {
      PointI off = -make_point(entry_dir.x, entry_dir.z);
      return make_pair(origin + off, -entry_dir);
   }
}

void SplineTrack::get_endpoints(PointList& output) const
{
   output.push_back(origin);

   if (abs(delta.x) > 0 || abs(delta.y) > 0)
      output.push_back(
         make_point(origin.x + delta.x, origin.y + delta.y));
}

void SplineTrack::get_covers(PointList& output) const
{
   const Point<float> off = make_point(0.5f, 0.5f);

   for (int x = min(0, delta.x); x <= max(0, delta.x) + 1; x++) {
      for (int y = min(0, delta.y); y <= max(0, delta.y) + 1; y++) {
         PointI p = make_point(x, y);

         const bool is_origin = p == make_point(0, 0);
         const bool is_delta = p == make_point(delta.x, delta.y);

         if (point_in_polygon(bounds, point_cast<float>(p) + off)
             && !(is_origin || is_delta))
            output.push_back(p + origin);
      }
   }
}

bool SplineTrack::point_in_polygon(const Polygon& poly, Point<float> p)
{
   bool odd_nodes = false;
   const int n_sides = poly.size();
   int j = n_sides - 1;

   const float x = p.x - 0.5f;
   const float y = p.y - 0.5f;

   for (int i = 0; i < n_sides; i++) {
      if ((poly[i].y < y && poly[j].y >= y)
          || (poly[j].y < y && poly[i].y >= y)) {

         if (poly[i].x + (y - poly[i].y)/(poly[j].y-poly[i].y)*(poly[j].x-poly[i].x) < x)
            odd_nodes = !odd_nodes;
      }
      j = i;
   }

   return odd_nodes;
}

void SplineTrack::bounding_polygon(Polygon& poly) const
{
   const float step = 0.01f;
   const float fudge = 0.8f;

   for (float t = 0.0f; t <= 1.0f; t += step) {
      Vector<float> v = curve.offset(t, fudge);
      poly.push_back(make_point(v.x, v.z));
   }

   for (float t = 1.0f; t >= 0.0f; t -= step) {
      Vector<float> v = curve.offset(t, -fudge);
      poly.push_back(make_point(v.x, v.z));
   }
}

void SplineTrack::get_height_locked(PointList& output) const
{
   for (int x = min(0, delta.x); x <= max(0, delta.x) + 1; x++) {
      for (int y = min(0, delta.y); y <= max(0, delta.y) + 1; y++) {

         PointI p = make_point(x, y);
         if (point_in_polygon(bounds, point_cast<float>(p)))
            output.push_back(p + origin);
      }
   }
}

ITrackSegmentPtr SplineTrack::merge_exit(PointI where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

float SplineTrack::rotation_at(float curve_delta) const
{
   assert(curve_delta >= 0.0f && curve_delta <= 1.0f);

   const Vector<float> deriv = curve.deriv(curve_delta);

   // Derivation of angle depends on quadrant
   if (deriv.z >= 0 && deriv.x > 0)
      return rad_to_deg<float>(atanf(deriv.z / deriv.x));
   else if (deriv.z > 0 && deriv.x <= 0)
      return 90 - rad_to_deg<float>(atanf(deriv.x / deriv.z));
   else if (deriv.z <= 0 && deriv.x <= 0)
      return 270 - rad_to_deg<float>(atanf(deriv.x / deriv.z));
   else if (deriv.z <= 0 && deriv.x > 0)
      return rad_to_deg<float>(atanf(deriv.z / deriv.x));
   else
      assert(false);
}

void SplineTrack::transform(const track::TravelToken& token,
                            float delta, bool backwards) const
{
   assert(delta < curve.length);

   const float curve_delta =
      (backwards ? curve.length - delta : delta) / curve.length;

   float u_curve_delta;
   Vector<float> curve_value = curve.linear(curve_delta, &u_curve_delta);

   glTranslatef(
      static_cast<float>(origin.x) + curve_value.x,
      height,
      static_cast<float>(origin.y) + curve_value.z);

   float angle = rotation_at(u_curve_delta);
   if (backwards)
      angle += 180.0f;

   glRotatef(-angle, 0.0f, 1.0f, 0.0f);
}

track::TravelToken SplineTrack::get_travel_token(track::Position pos,
                                                 track::Direction dir) const
{
   using namespace placeholders;

   ensure_valid_direction(dir);

   const bool backwards = dir == -exit_dir;

   track::TravelToken tok = {
      dir,
      pos,
      bind(&SplineTrack::transform, this, _1, _2, backwards),
      track::flat_gradient_func,
      1
   };
   return tok;
}

xml::element SplineTrack::to_xml() const
{
   return xml::element("spline-track")
      .add_attribute("delta-x", delta.x)
      .add_attribute("delta-y", delta.y)
      .add_attribute("entry-dir-x", entry_dir.x)
      .add_attribute("entry-dir-y", entry_dir.z)
      .add_attribute("exit-dir-x", exit_dir.x)
      .add_attribute("exit-dir-y", exit_dir.z);
}

ITrackSegmentPtr make_spline_track(VectorI delta,
                                   track::Direction entry_dir,
                                   track::Direction exit_dir)
{
   return ITrackSegmentPtr(new SplineTrack(delta, entry_dir, exit_dir));
}
