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
#include "OpenGLHelper.hpp"
#include "ILogger.hpp"
#include "Matrix.hpp"
#include "SolveCubic.hpp"

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
   void render() const;
   void merge(IMeshBufferPtr buf) const;
   void set_origin(int x, int y, float h);
   float segment_length(const track::TravelToken& token) const;
   bool is_valid_direction(const track::Direction& dir) const;
   track::Connection next_position(const track::TravelToken& token) const;
   void get_endpoints(vector<Point<int> >& output) const;
   void get_covers(vector<Point<int> >& output) const;
   void get_covers2(vector<Point<int> >& output) const;
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
   typedef vector<Point<float> > Polygon;
   void bounding_polygon(Polygon& poly) const;
   
   float extend_from_center(track::Direction dir) const;
   void ensure_valid_direction(track::Direction dir) const;
   void transform(const track::TravelToken& token, float delta) const;
   float rotation_at(float delta) const;
   
   BezierCurve<float> curve;
   IMeshBufferPtr rail_buf;

   Point<int> origin;
   float height;
   Vector<int> delta;
   track::Direction entry_dir, exit_dir;

   typedef tuple<Vector<int>,
                 track::Direction,
                 track::Direction> Parameters;
   typedef map<Parameters, IMeshBufferPtr> MeshCache; 
   static MeshCache mesh_cache;

   typedef set<Point<int> > PointSet;
};

GenTrack::MeshCache GenTrack::mesh_cache;

GenTrack::GenTrack(Vector<int> delta,
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
   Vector<float> p2 = entry_dir_norm * pinch_length;
   Vector<float> p3 = delta_f - (exit_dir_norm * pinch_length);
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
}

float GenTrack::extend_from_center(track::Direction dir) const
{   
   // Track must extend from the centre to the edge of a tile
   const float x_sq = static_cast<float>(dir.x * dir.x);
   const float y_sq = static_cast<float>(dir.z * dir.z);
   
   if (dir.x == dir.z)
      return sqrtf(2.0f) * 0.5f;
   else if (dir.x < dir.z)
      return sqrtf(x_sq / y_sq + 1) * 0.5f;
   else
      return sqrtf(y_sq / x_sq + 1) * 0.5f;   
}

void GenTrack::merge(IMeshBufferPtr buf) const
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
      
      Vector<float> v = curve(pos / curve.length);

      const Vector<float> deriv = curve.deriv(pos / curve.length);
      const float angle =
         rad_to_deg<float>(atanf(deriv.z / deriv.x));

      merge_sleeper(buf, off + v, -angle);
   }
}

void GenTrack::render() const
{
   Polygon p;
   bounding_polygon(p);

   glPushMatrix();

   glTranslatef(origin.x, 0.0f, origin.y);
   glColor3f(0.1f, 0.1f, 0.8f);
   
   glBegin(GL_LINE_LOOP);
   for (Polygon::const_iterator it = p.begin(); it != p.end(); ++it) {
      glVertex3f((*it).x, 0.1f, (*it).y);
   }
   glEnd();

   glPopMatrix();
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
   return dir == entry_dir || dir == -exit_dir;
}

void GenTrack::ensure_valid_direction(track::Direction dir) const
{
   if (!is_valid_direction(dir))
      throw runtime_error(
         "Invalid direction on gen-track: "
         + boost::lexical_cast<string>(dir)
         + " (should be "
         + boost::lexical_cast<string>(entry_dir)
         + " or " + boost::lexical_cast<string>(-exit_dir) + ")");
}

track::Connection GenTrack::next_position(const track::TravelToken& token) const
{
   ensure_valid_direction(token.direction);

   if (token.direction == entry_dir) {
      Point<int> off =
         make_point(exit_dir.x, exit_dir.z) + make_point(delta.x, delta.y);
      return make_pair(origin + off, exit_dir);
   }
   else {
      Point<int> off = -make_point(exit_dir.x, exit_dir.z);
      return make_pair(origin + off, exit_dir);
   }   
}

void GenTrack::get_endpoints(vector<Point<int> >& output) const
{
   output.push_back(origin);

   if (abs(delta.x) > 0 || abs(delta.y) > 0)
      output.push_back(
         make_point(origin.x + delta.x, origin.y + delta.y));
}

void GenTrack::get_covers(vector<Point<int> >& output) const
{
   
}

void GenTrack::bounding_polygon(Polygon& poly) const
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

void GenTrack::get_covers2(vector<Point<int> >& output) const
{
   PointSet all;

   for (int x = min(0, delta.x); x <= max(0, delta.x) + 1; x++) {
      for (int y = min(0, delta.y); y <= max(0, delta.y) + 1; y++)
         all.insert(origin + make_point(x, y));
   }

   copy(all.begin(), all.end(), back_inserter(output));
}

ITrackSegmentPtr GenTrack::merge_exit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

float GenTrack::rotation_at(float curve_delta) const
{
   assert(curve_delta >= 0.0f && curve_delta <= 1.0f);
   
   const Vector<float> deriv = curve.deriv(curve_delta);

   // Derivation of angle depends on quadrant
   if (deriv.z >= 0 && deriv.x > 0)
      return rad_to_deg<float>(atanf(deriv.z / deriv.x));
   else if (deriv.z > 0 && deriv.x <= 0)
      return 90 - rad_to_deg<float>(atanf(deriv.x / deriv.z));
   else if (deriv.z < 0 && deriv.x <= 0)
      return 270 - rad_to_deg<float>(atanf(deriv.x / deriv.z));
   else if (deriv.z <= 0 && deriv.x > 0)
      return rad_to_deg<float>(atanf(deriv.z / deriv.x));
   else
      assert(false);
}

void GenTrack::transform(const track::TravelToken& token, float delta) const
{
   assert(delta < curve.length);

   const float curve_delta = delta / curve.length;

   Vector<float> curve_value = curve(curve_delta);

   const float angle = rotation_at(curve_delta);
   
   glTranslatef(
      static_cast<float>(origin.x) + curve_value.x,
      height,
      static_cast<float>(origin.y) + curve_value.z);
      
   glRotatef(-angle, 0.0f, 1.0f, 0.0f);

}

track::TravelToken GenTrack::get_travel_token(track::Position pos,
                                              track::Direction dir) const
{
   using namespace placeholders;

   ensure_valid_direction(dir);

   track::TravelToken tok = {
      dir,
      pos,
      bind(&GenTrack::transform, this, _1, _2),
      track::flat_gradient_func,
      1
   };
   return tok;
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
