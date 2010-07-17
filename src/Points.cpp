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
#include "XMLBuilder.hpp"
#include "ILogger.hpp"
#include "BezierCurve.hpp"
#include "Matrix.hpp"

#include <cassert>

#include <GL/gl.h>
#include <boost/lexical_cast.hpp>

// Forks in the track
class Points : public ITrackSegment,
               private StraightTrackHelper,
               private SleeperHelper,
               private BezierHelper {
public:
   Points(track::Direction a_direction, bool reflect);

   // ITrackSegment interface
   void render() const;
   void merge(IMeshBufferPtr buf) const;
   void set_origin(int x, int y, float h) { myX = x; myY = y; height = h; }
   float segment_length(const track::TravelToken& a_token) const;
   bool is_valid_direction(const track::Direction& a_direction) const;
   track::Connection next_position(const track::TravelToken& a_token) const;
   void get_endpoints(vector<Point<int> >& a_list) const;
   void get_covers(vector<Point<int> >& output) const;
   void get_covers2(vector<Point<int> >& output) const;
   ITrackSegmentPtr merge_exit(Point<int> where, track::Direction dir);
   track::TravelToken get_travel_token(track::Position a_position,
      track::Direction a_direction) const;
   void next_state();
   void prev_state();
   bool has_multiple_states() const { return true; }
   void set_state_render_hint();

   // IXMLSerialisable interface
   xml::element to_xml() const;
private:
   void transform(const track::TravelToken& a_token, float a_delta) const;
   void ensure_valid_direction(track::Direction a_direction) const;
   void render_arrow() const;

   Point<int> displaced_endpoint() const;
   Point<int> straight_endpoint() const;

   enum State { TAKEN, NOT_TAKEN };
   
   int myX, myY;
   track::Direction my_axis;
   bool reflected;
   State state;
   float height;

   // Draw the arrow over the points if true
   mutable bool state_render_hint;

   static const BezierCurve<float> my_curve, my_reflected_curve;
};

const BezierCurve<float> Points::my_curve = make_bezier_curve
   (make_vector(0.0f, 0.0f, 0.0f),
      make_vector(1.0f, 0.0f, 0.0f),
      make_vector(2.0f, 0.0f, 1.0f),
      make_vector(3.0f, 0.0f, 1.0f));

const BezierCurve<float> Points::my_reflected_curve = make_bezier_curve
   (make_vector(0.0f, 0.0f, 0.0f),
      make_vector(1.0f, 0.0f, 0.0f),
      make_vector(2.0f, 0.0f, -1.0f),
      make_vector(3.0f, 0.0f, -1.0f));
      
Points::Points(track::Direction a_direction, bool reflect)
   : myX(0), myY(0),
     my_axis(a_direction), reflected(reflect),
     state(NOT_TAKEN),
     height(0.0f),
     state_render_hint(false)
{
   
}

void Points::set_state_render_hint() 
{
   state_render_hint = true;
}

void Points::render_arrow() const
{
   glPushMatrix();
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_BLEND);

   glTranslatef(-0.5f, 0.11f, 0.0f);
   glColor4f(0.2f, 0.1f, 0.9f, 0.7f);

   const float head_width = 0.25f;
    
   if (state == TAKEN) {
	
      const BezierCurve<float>& curve =
         reflected ? my_reflected_curve : my_curve;

      const float step = 0.1f;
      const float arrow_len = 0.7f;

      glDisable(GL_CULL_FACE);

      for (float t = 0.0f; t < arrow_len; t += step) {

         const Vector<float> v1 = curve(t);
         const Vector<float> v2 = curve(t + step);

         if (t >= arrow_len - step) {
            // Arrow head
            glBegin(GL_TRIANGLES);
            {
               glVertex3f(v1.x, 0.0f, v1.z - head_width);
               glVertex3f(v2.x, 0.0f, v2.z);
               glVertex3f(v1.x, 0.0f, v1.z + head_width);
            }
            glEnd();
         }
         else {
            glBegin(GL_QUADS);
            {
               glVertex3f(v1.x, 0.0f, v1.z - 0.1f);
               glVertex3f(v1.x, 0.0f, v1.z + 0.1f);
               glVertex3f(v2.x, 0.0f, v2.z + 0.1f);
               glVertex3f(v2.x, 0.0f, v2.z - 0.1f);
            }
            glEnd();
         }
      }
   }
   else {
      const float head_length = 0.3f;
	
      glBegin(GL_QUADS);
      {
         glVertex3f(0.0f, 0.0f, 0.1f);
         glVertex3f(2.0f - head_length, 0.0f, 0.1f);
         glVertex3f(2.0f - head_length, 0.0f, -0.1f);
         glVertex3f(0.0f, 0.0f, -0.1f);
      }
      glEnd();
	
      // Draw the arrow head
      glBegin(GL_TRIANGLES);
      {
         glVertex3f(2.0f - head_length, 0.0f, head_width);
         glVertex3f(2.0f, 0.0f, 0.0f);
         glVertex3f(2.0f - head_length, 0.0f, -head_width);
      }
      glEnd();
   }

   glPopAttrib();
   glPopMatrix();
}

void Points::merge(IMeshBufferPtr buf) const
{
   static IMeshBufferPtr rail_buf = make_bezier_rail_mesh(my_curve);
   static IMeshBufferPtr reflect_buf = make_bezier_rail_mesh(my_reflected_curve);
   
   Vector<float> off = make_vector(
      static_cast<float>(myX),
      height,
      static_cast<float>(myY));
   
   float y_angle = 0.0f;
      
   if (my_axis == -axis::X)
      y_angle = 180.0f;
   else if (my_axis == -axis::Y)
      y_angle = 90.0f;
   else if (my_axis == axis::Y)
      y_angle = 270.0f;

   // Render the rails
   
   buf->merge(reflected ? reflect_buf : rail_buf,
      off + rotateY(make_vector(-0.5f, 0.0f, 0.0f), y_angle),
      y_angle);
   
   {
      Vector<float> t = off;
      
      for (int i = 0; i < 3; i++) {
         const float a = y_angle + 90.0f;
         merge_straight_rail(buf, t, a);
         
         t += rotateY(make_vector(0.0f, 0.0f, 1.0f), a);
      }
   }

   // Draw the curved sleepers
   for (float i = 0.25f; i < 1.0f; i += 0.08f) {
      Vector<float> v = (reflected ? my_reflected_curve : my_curve)(i);

      Vector<float> t = make_vector(v.x - 0.5f, 0.0f, v.z);
      Vector<float> soff = off + rotateY(t, y_angle);
      const Vector<float> deriv =
         (reflected ? my_reflected_curve : my_curve).deriv(i);
      const float angle =
         rad_to_deg<float>(atanf(deriv.z / deriv.x));

      merge_sleeper(buf, soff, y_angle - angle);
   }
   
   // Draw the straight sleepers
   off -= rotateY(make_vector(0.4f, 0.0f, 0.0f), y_angle);
   
   for (int i = 0; i < 12; i++) {
      merge_sleeper(buf, off, y_angle);
      off += rotateY(make_vector(0.25f, 0.0f, 0.0f), y_angle);
   }
}

void Points::render() const
{
   if (state_render_hint) {
      glPushMatrix();
      
      glTranslatef(
         static_cast<float>(myX),
         height,
         static_cast<float>(myY));

      if (my_axis == -axis::X)
         glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
      else if (my_axis == -axis::Y)
         glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
      else if (my_axis == axis::Y)
         glRotatef(270.0f, 0.0f, 1.0f, 0.0f);
         
      render_arrow();
      state_render_hint = false;
      
      glPopMatrix();
   }
}

float Points::segment_length(const track::TravelToken& a_token) const
{
   if (a_token.position == displaced_endpoint())
      return my_curve.length;
   else
      return 3.0f;
}

track::TravelToken Points::get_travel_token(track::Position position,
   track::Direction direction) const
{
   using namespace placeholders;
   
   ensure_valid_direction(direction);

   const int n_exits = position.x == myX && position.y == myY ? 2 : 1;
    
   track::TravelToken tok = {
      direction,
      position,
      bind(&Points::transform, this, _1, _2),
      track::flat_gradient_func,
      n_exits
   };
    
   return tok;
}

void Points::transform(const track::TravelToken& a_token, float delta) const
{
   const float len = segment_length(a_token);
   
   assert(delta < len);

   if (myX == a_token.position.x && myY == a_token.position.y
      && state == NOT_TAKEN) {

      if (a_token.direction == my_axis
         && (my_axis == -axis::X || my_axis == -axis::Y))
         delta -= 1.0f;
      
      const float x_trans =
         my_axis == axis::X ? delta
         : (my_axis == -axis::X ? -delta : 0.0f);
      const float y_trans =
         my_axis == axis::Y ? delta
         : (my_axis == -axis::Y ? -delta : 0.0f);
      
      glTranslatef(static_cast<float>(myX) + x_trans,
         height,
         static_cast<float>(myY) + y_trans);
      
      if (my_axis == axis::Y || my_axis == -axis::Y)
         glRotated(-90.0, 0.0, 1.0, 0.0);
      
      glTranslated(-0.5, 0.0, 0.0);
   }
   else if (a_token.position == straight_endpoint()) {
      delta = 2.0f - delta;

      if (a_token.direction == -my_axis
         && (my_axis == axis::X || my_axis == axis::Y))
         delta += 1.0f;
      
      const float x_trans =
         my_axis == axis::X ? delta
         : (my_axis == -axis::X ? -delta : 0.0f);
      const float y_trans =
         my_axis == axis::Y ? delta
         : (my_axis == -axis::Y ? -delta : 0.0f);
      
      glTranslatef(static_cast<float>(myX) + x_trans,
         height,
         static_cast<float>(myY) + y_trans);
      
      if (my_axis == axis::Y || my_axis == -axis::Y)
         glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
      
      glTranslatef(-0.5f, 0.0f, 0.0f);
   }
   else if (a_token.position == displaced_endpoint() || state == TAKEN) {
      // Curving onto the straight section
      float x_trans, y_trans, rotate;

      // We have a slight problem in that the domain of the curve
      // function is [0,1] but the delta is in [0,len] so we have
      // to compress the delta into [0,1] here
      const float curve_delta = delta / len;

      bool backwards = a_token.position == displaced_endpoint();
      
      const float f_value = backwards ? 1.0f - curve_delta : curve_delta;
      const Vector<float> curve_value = my_curve(f_value);
      
      // Calculate the angle that the tangent to the curve at this
      // point makes to (one of) the axis at this point
      const Vector<float> deriv = my_curve.deriv(f_value);
      const float angle =
         rad_to_deg<float>(atanf(deriv.z / deriv.x));

      if (my_axis == -axis::X) {
         x_trans = 1.0f - curve_value.x;
         y_trans = reflected ? curve_value.z : -curve_value.z;
         rotate = reflected ? angle : -angle;
      }
      else if (my_axis == axis::X) {
         x_trans = curve_value.x;
         y_trans = reflected ? -curve_value.z : curve_value.z;
         rotate = reflected ? angle : -angle;
      }
      else if (my_axis == -axis::Y) {
         x_trans = reflected ? -curve_value.z : curve_value.z;
         y_trans = 1.0f - curve_value.x;
         rotate = reflected ? angle : -angle;
      }
      else if (my_axis == axis::Y) {
         x_trans = reflected ? curve_value.z: -curve_value.z;
         y_trans = curve_value.x;
         rotate = reflected ? angle : -angle;
      }
      else
         assert(false);

      glTranslatef(
         static_cast<float>(myX) + x_trans,
         height,
         static_cast<float>(myY) + y_trans);
      
      if (my_axis == axis::Y || my_axis == -axis::Y)
         glRotatef(-90.0f, 0.0f, 1.0f, 0.0f);
      
      glTranslatef(-0.5f, 0.0f, 0.0f);

      glRotatef(rotate, 0.0f, 1.0f, 0.0f);
   }
   else
      assert(false);
   
   if (a_token.direction == -axis::X || a_token.direction == -axis::Y)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

void Points::ensure_valid_direction(track::Direction a_direction) const
{
   if (!is_valid_direction(a_direction))
      throw runtime_error
         ("Invalid direction on points: "
            + boost::lexical_cast<string>(a_direction)
            + " (should be parallel to "
            + boost::lexical_cast<string>(my_axis) + ")");
}

bool Points::is_valid_direction(const track::Direction& a_direction) const
{
   if (my_axis == axis::X || my_axis == -axis::X)
      return a_direction == axis::X || -a_direction == axis::X;
   else
      return a_direction == axis::Y || -a_direction == axis::Y;
}

track::Connection Points::next_position(const track::TravelToken& a_token) const
{
   const bool branching = state == TAKEN;
         
   if (my_axis == axis::X) {
      if (a_token.direction == -axis::X) {
         // Two possible entry points
         return make_pair(make_point(myX - 1, myY), -axis::X);
      }
      else {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(make_point(myX + 3, myY - 1), axis::X);
            else
               return make_pair(make_point(myX + 3, myY + 1), axis::X);
         }
         else
            return make_pair(make_point(myX + 3, myY), axis::X);
      }
   }
   else if (my_axis == -axis::X) {
      if (a_token.direction == -axis::X) {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(make_point(myX - 3, myY + 1), -axis::X);
            else
               return make_pair(make_point(myX - 3, myY - 1), -axis::X);
         }
         else
            return make_pair(make_point(myX - 3, myY), -axis::X);
      }
      else {
         // Two possible entry points
         return make_pair(make_point(myX + 1, myY), axis::X);
      }
   }
   else if (my_axis == axis::Y) {
      if (a_token.direction == -axis::Y) {
         // Two possible entry points
         return make_pair(make_point(myX, myY - 1), -axis::Y);
      }
      else {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(make_point(myX + 1, myY + 3), axis::Y);
            else
               return make_pair(make_point(myX - 1, myY + 3), axis::Y);
         }
         else
            return make_pair(make_point(myX, myY + 3), axis::Y);
      }
   }
   else if (my_axis == -axis::Y) {
      if (a_token.direction == -axis::Y) {
         // Two possible exits
         if (branching) {
            if (reflected)
               return make_pair(make_point(myX - 1, myY - 3), -axis::Y);
            else
               return make_pair(make_point(myX + 1, myY - 3), -axis::Y);
         }
         else
            return make_pair(make_point(myX, myY - 3), -axis::Y);
      }
      else {
         // Two possible entry points
         return make_pair(make_point(myX, myY + 1), axis::Y);
      }
   }
   else
      assert(false);
}

// Get the endpoint that follows the curve
Point<int> Points::displaced_endpoint() const
{
   const int reflect = reflected ? -1 : 1;

   if (my_axis == axis::X)
      return make_point(myX + 2, myY + 1*reflect);
   else if (my_axis == -axis::X)
      return make_point(myX - 2, myY - 1*reflect);
   else if (my_axis == axis::Y)
      return make_point(myX - 1*reflect, myY + 2);
   else if (my_axis == -axis::Y)
      return make_point(myX + 1*reflect, myY - 2);
   else
      assert(false);
}

// Get the endpoint that follows the straight track
Point<int> Points::straight_endpoint() const
{
   if (my_axis == axis::X)
      return make_point(myX + 2, myY);
   else if (my_axis == -axis::X)
      return make_point(myX - 2, myY);
   else if (my_axis == axis::Y)
      return make_point(myX, myY + 2);
   else if (my_axis == -axis::Y)
      return make_point(myX, myY - 2);
   else
      assert(false);
}

void Points::get_endpoints(vector<Point<int> >& a_list) const
{
   a_list.push_back(make_point(myX, myY));
   a_list.push_back(straight_endpoint());
   a_list.push_back(displaced_endpoint());
}

void Points::get_covers(vector<Point<int> >& output) const
{
   const int reflect = reflected ? -1 : 1;

   if (my_axis == axis::X) {
      output.push_back(make_point(myX + 1, myY + 1*reflect));
      output.push_back(make_point(myX + 1, myY));
   }
   else if (my_axis == -axis::X) {
      output.push_back(make_point(myX - 1, myY - 1*reflect));
      output.push_back(make_point(myX - 1, myY));
   }
   else if (my_axis == axis::Y) {
      output.push_back(make_point(myX - 1*reflect, myY + 1));
      output.push_back(make_point(myX, myY + 1));
   }
   else if (my_axis == -axis::Y) {
      output.push_back(make_point(myX + 1*reflect, myY - 1));
      output.push_back(make_point(myX, myY - 1));
   }
   else
      assert(false);
}

void Points::get_covers2(vector<Point<int> >& output) const
{

}

ITrackSegmentPtr Points::merge_exit(Point<int> where, track::Direction dir)
{
   // Cant merge with anything
   return ITrackSegmentPtr();
}

xml::element Points::to_xml() const
{ 
   return xml::element("points")
      .add_attribute("align",
         my_axis == axis::X ? "x"
         : (my_axis == -axis::X ? "-x"
            : (my_axis == axis::Y ? "y"
               : (my_axis == -axis::Y ? "-y" : "?"))))
      .add_attribute("reflect", reflected);
}

void Points::next_state()
{
   state = reflected ? NOT_TAKEN : TAKEN;
}

void Points::prev_state()
{
   state = reflected ? TAKEN : NOT_TAKEN;
}

ITrackSegmentPtr make_points(track::Direction a_direction, bool reflect)
{
   return ITrackSegmentPtr(new Points(a_direction, reflect));
}
