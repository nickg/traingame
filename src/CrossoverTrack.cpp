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
#include "OpenGLHelper.hpp"

#include <stdexcept>
#include <cassert>

#include <boost/lexical_cast.hpp>

using namespace placeholders;
using namespace boost;

// A section of track that allows travelling along both axis
class CrossoverTrack : public ITrackSegment,
                       public enable_shared_from_this<CrossoverTrack>,
                       private StraightTrackHelper,
                       private SleeperHelper {
public:
   CrossoverTrack() : height(0.0f) {}
   ~CrossoverTrack() {}

   void set_origin(int x, int y, float h);
   
   void render() const {}
   void merge(IMeshBufferPtr buf) const;
   
   float segment_length(const track::TravelToken& a_token) const;
   bool is_valid_direction(const track::Direction& a_direction) const;
   track::Connection next_position(const track::TravelToken& a_token) const;
   void get_endpoints(vector<Point<int> >& a_list) const;
   void get_covers(vector<Point<int> >& output) const { }
   void get_height_locked(vector<Point<int> >& output) const;
   ITrackSegmentPtr merge_exit(Point<int> where, track::Direction dir);
   track::TravelToken get_travel_token(track::Position a_position,
      track::Direction a_direction) const;
   void next_state() {}
   void prev_state() {}
   bool has_multiple_states() const { return false; }
   void set_state_render_hint() {}

   // IXMLSerialisable interface
   xml::element to_xml() const;
   
private:
   void transform(const track::TravelToken& a_token, float delta) const;
   
   Point<int> origin;
   float height;
};

void CrossoverTrack::merge(IMeshBufferPtr buf) const
{
   // Render the y-going rails and sleepers
   {    
      Vector<float> off = make_vector(
         static_cast<float>(origin.x),
         height,
         static_cast<float>(origin.y));
      
      merge_straight_rail(buf, off, 0.0f);

      off += make_vector(0.0f, 0.0f, -0.4f);
            
      for (int i = 0; i < 4; i++) {
         merge_sleeper(buf, off, 90.0f);
         off += make_vector(0.0f, 0.0f, 0.25f);
      }
   }

   // Render the x-going rails and sleepers
   {
      Vector<float> off = make_vector(
         static_cast<float>(origin.x),
         height,
         static_cast<float>(origin.y));
      
      merge_straight_rail(buf, off, 90.0f);
      
      off += make_vector(-0.4f, 0.0f, 0.0f);
            
      for (int i = 0; i < 4; i++) {
         merge_sleeper(buf, off, 0.0f);
         off += make_vector(0.25f, 0.0f, 0.0f);
      }
   }
}

void CrossoverTrack::set_origin(int x, int y, float h )
{
   origin = make_point(x, y);
   height = h;
}

float CrossoverTrack::segment_length(const track::TravelToken& a_token) const
{
   return 1.0f;
}

track::TravelToken
CrossoverTrack::get_travel_token(track::Position a_position,
   track::Direction a_direction) const
{
   if (!is_valid_direction(a_direction))
      throw runtime_error
         ("Invalid direction on crossover: " + lexical_cast<string>(a_direction));

   track::TravelToken tok = {
      a_direction,
      a_position,
      bind(&CrossoverTrack::transform, this, _1, _2),
      track::flat_gradient_func,
      1
   };
   return tok;
}

void CrossoverTrack::transform(const track::TravelToken& a_token,
   float delta) const
{
   assert(delta < 1.0);
   
   bool backwards = a_token.direction== -axis::X || a_token.direction == -axis::Y;

   if (backwards) {
      delta = 1.0f - delta;
   }

   track::Direction dir = backwards ? -a_token.direction : a_token.direction;

   const double x_trans = dir == axis::X ? delta : 0;
   const double y_trans = dir == axis::Y ? delta : 0;

   glTranslated(static_cast<double>(origin.x) + x_trans,
      height,
      static_cast<double>(origin.y) + y_trans);

   if (dir == axis::Y)
      glRotated(-90.0, 0.0, 1.0, 0.0);

   glTranslated(-0.5, 0.0, 0.0);
   
   if (backwards)
      glRotated(-180.0, 0.0, 1.0, 0.0);
}

bool CrossoverTrack::is_valid_direction(const track::Direction& a_direction) const
{
   return a_direction == axis::X || a_direction == axis::Y
      || a_direction == -axis::Y || a_direction == -axis::X;
}

track::Connection
CrossoverTrack::next_position(const track::TravelToken& a_token) const
{
   if (a_token.direction == axis::X)
      return make_pair(origin + make_point(1, 0), axis::X);
   else if (a_token.direction == -axis::X)
      return make_pair(origin - make_point(1, 0), -axis::X);
   else if (a_token.direction == axis::Y)
      return make_pair(origin + make_point(0, 1), axis::Y);
   else if (a_token.direction == -axis::Y)
      return make_pair(origin - make_point(0, 1), -axis::Y);
   else
      throw runtime_error
         ("Invalid direction on crossover: " + lexical_cast<string>(a_token.direction));
}

void CrossoverTrack::get_endpoints(vector<Point<int> >& a_list) const
{
   a_list.push_back(origin);
}

void CrossoverTrack::get_height_locked(vector<Point<int> >& output) const
{   
   output.push_back(origin + make_point(0, 0));
   output.push_back(origin + make_point(0, 1));
   output.push_back(origin + make_point(1, 1));
   output.push_back(origin + make_point(1, 0));
}

ITrackSegmentPtr CrossoverTrack::merge_exit(Point<int> where,
   track::Direction dir)
{
   if (where == origin
      && is_valid_direction(dir))
      return shared_from_this();

   // No way to extend a crossover
   return ITrackSegmentPtr();
}

xml::element CrossoverTrack::to_xml() const
{
   return xml::element("crossover-track");
}

ITrackSegmentPtr make_crossover_track()
{
   return ITrackSegmentPtr(static_cast<ITrackSegment *>(new CrossoverTrack));
}
