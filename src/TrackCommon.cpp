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

#include "TrackCommon.hpp"
#include "ILogger.hpp"
#include "Maths.hpp"
#include "IMesh.hpp"
#include "Matrix.hpp"

#include <cmath>
#include <map>

namespace {
   const float RAIL_WIDTH = 0.05f;
   const float GAUGE = 0.5f;

   const int SLEEPERS_PER_UNIT = 4;

   const float SLEEPER_LENGTH = 0.8f;

   const Colour METAL = make_colour(0.5f, 0.5f, 0.5f);
}

IMeshBufferPtr SleeperHelper::sleeper_buf;

IMeshBufferPtr SleeperHelper::generate_sleeper_mesh_buffer()
{
   IMeshBufferPtr buf = make_mesh_buffer();

   const Colour brown = make_colour(0.5f, 0.3f, 0.0f);

   const float sleeper_width = 0.1f;
   const float sleeper_depth = 0.05f;
   const float sleeper_off = sleeper_width / 2.0f;

   const float r = SLEEPER_LENGTH / 2.0f;

   // Top
   buf->add_quad(make_vector(-sleeper_off, sleeper_depth, -r),
      make_vector(-sleeper_off, sleeper_depth, r),
      make_vector(sleeper_off, sleeper_depth, r),
      make_vector(sleeper_off, sleeper_depth, -r),
      brown);

   // Side 1
   buf->add_quad(make_vector(sleeper_off, sleeper_depth, -r),
      make_vector(sleeper_off, 0.0f, -r),
      make_vector(-sleeper_off, 0.0f, -r),
      make_vector(-sleeper_off, sleeper_depth, -r),
      brown);

   // Side 2
   buf->add_quad(make_vector(-sleeper_off, sleeper_depth, r),
      make_vector(-sleeper_off, 0.0f, r),
      make_vector(sleeper_off, 0.0f, r),
      make_vector(sleeper_off, sleeper_depth, r),
      brown);

   // Front
   buf->add_quad(make_vector(sleeper_off, 0.0f, r),
      make_vector(sleeper_off, 0.0f, -r),
      make_vector(sleeper_off, sleeper_depth, -r),
      make_vector(sleeper_off, sleeper_depth, r),
      brown);

   // Back
   buf->add_quad(make_vector(-sleeper_off, sleeper_depth, r),
      make_vector(-sleeper_off, sleeper_depth, -r),
      make_vector(-sleeper_off, 0.0f, -r),
      make_vector(-sleeper_off, 0.0f, r),
      brown);

   return buf;
}

void SleeperHelper::merge_sleeper(IMeshBufferPtr buf,
   Vector<float> off, float y_angle) const
{
   if (!sleeper_buf)
      sleeper_buf = generate_sleeper_mesh_buffer();

   buf->merge(sleeper_buf, off, y_angle);
}

void BezierHelper::build_one_bezier_rail(const BezierCurve<float>& func,
   IMeshBufferPtr buf, float p)
{
   const float step = 0.1f;

   for (float t = 0.0f; t < 1.0f; t += step) {

      const float half_rail = RAIL_WIDTH / 2.0f;

      Vector<float> v1_out = func.offset(t, p + half_rail);
      Vector<float> v2_out = func.offset(t + step, p + half_rail);

      Vector<float> v1_in = func.offset(t, p - half_rail);
      Vector<float> v2_in = func.offset(t + step, p - half_rail);

      // Top of rail
      buf->add_quad(
         make_vector(v1_out.x, v1_out.y + track::RAIL_HEIGHT, v1_out.z),
         make_vector(v1_in.x, v1_in.y + track::RAIL_HEIGHT, v1_in.z),
         make_vector(v2_in.x, v2_in.y + track::RAIL_HEIGHT, v2_in.z),
         make_vector(v2_out.x, v2_out.y + track::RAIL_HEIGHT, v2_out.z),
         METAL);

      // Outer edge
      buf->add_quad(
         make_vector(v2_out.x, v2_out.y + track::RAIL_HEIGHT, v2_out.z),
         make_vector(v2_out.x , v2_out.y, v2_out.z),
         make_vector(v1_out.x, v1_out.y, v1_out.z),
         make_vector(v1_out.x, v1_out.y + track::RAIL_HEIGHT, v1_out.z),
         METAL);

      // Inner edge
      buf->add_quad(
         make_vector(v1_in.x, v1_in.y + track::RAIL_HEIGHT, v1_in.z),
         make_vector(v1_in.x, v1_in.y, v1_in.z),
         make_vector(v2_in.x , v2_in.y, v2_in.z),
         make_vector(v2_in.x, v2_in.y + track::RAIL_HEIGHT, v2_in.z),
         METAL);
   }
}

IMeshBufferPtr BezierHelper::make_bezier_rail_mesh(
   const BezierCurve<float>& func) const
{
   IMeshBufferPtr buf = make_mesh_buffer();

   build_one_bezier_rail(func, buf, GAUGE/2.0f);
   build_one_bezier_rail(func, buf, -GAUGE/2.0f);

   return buf;
}

IMeshBufferPtr StraightTrackHelper::rail_buf;

IMeshBufferPtr StraightTrackHelper::generate_rail_mesh_buffer()
{
   IMeshBufferPtr buf = make_mesh_buffer();

   // Top side
   buf->add_quad(make_vector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      make_vector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      make_vector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      make_vector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      METAL);

   // Outer side
   buf->add_quad(make_vector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      make_vector(-RAIL_WIDTH/2.0f, 0.0f, 0.0f),
      make_vector(-RAIL_WIDTH/2.0f, 0.0f, 1.0f),
      make_vector(-RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      METAL);

   // Inner side
   buf->add_quad(make_vector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 1.0f),
      make_vector(RAIL_WIDTH/2.0f, 0.0f, 1.0f),
      make_vector(RAIL_WIDTH/2.0f, 0.0f, 0.0f),
      make_vector(RAIL_WIDTH/2.0f, track::RAIL_HEIGHT, 0.0f),
      METAL);

   return buf;
}

void StraightTrackHelper::merge_one_rail(IMeshBufferPtr buf,
   Vector<float> off, float y_angle) const
{
   if (!rail_buf)
      rail_buf = generate_rail_mesh_buffer();

   buf->merge(rail_buf, off, y_angle);
}

void StraightTrackHelper::merge_straight_rail(IMeshBufferPtr buf,
   Vector<float> off, float y_angle) const
{
   MatrixF4 r = MatrixF4::rotation(y_angle, MatrixF4::AXIS_Y);

   off += r.transform(make_vector(-GAUGE/2.0f, 0.0f, -0.5f));
   merge_one_rail(buf, off, y_angle);

   off += r.transform(make_vector(GAUGE, 0.0f, 0.0f));
   merge_one_rail(buf, off, y_angle);
}

CurvedTrackHelper::CurvedRailMeshMap CurvedTrackHelper::curved_rail_meshes;

// Move to the origin of a curved section of track
void CurvedTrackHelper::transform_to_origin(Vector<float>& off,
   int base_radius, track::Angle start_angle) const
{
   off += make_vector(
      (base_radius-1)*-sin(deg_to_rad(start_angle)) - 0.5f,
      0.0f,
      (base_radius-1)*-cos(deg_to_rad(start_angle)) - 0.5f);

   // There *must* be a way to incorporate this in the above translation
   // as a neat formula, but I really can't think of it
   // This is a complete a hack, but whatever...
   if (start_angle >= 90 && start_angle <= 180)
      off += make_vector(0.0f, 0.0f, 1.0f);

   if (start_angle >= 180 && start_angle <= 270)
      off += make_vector(1.0f, 0.0f, 0.0f);
}

void CurvedTrackHelper::generate_curved_rail_mesh(IMeshBufferPtr buf,
   int base_radius, RailType type)
{
   const float edge_width = (1 - GAUGE - RAIL_WIDTH)/2.0f;
   const float R = static_cast<float>(base_radius) - edge_width
      - (type == OUTER_RAIL ? 0 : GAUGE);
   const float r = R - RAIL_WIDTH;

   const float step = M_PI / 2.0f / 10.0f;

   // Top of rail
   for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
      buf->add_quad(make_vector(r * cos(theta), 0.1f, r * sin(theta)),
         make_vector(r * cos(theta + step), 0.1f, r * sin(theta + step)),
         make_vector(R * cos(theta + step), 0.1f, R * sin(theta + step)),
         make_vector(R * cos(theta), 0.1f, R * sin(theta)),
         METAL);
   }

   // Outer edge
   for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
      const float sinT = sin(theta);
      const float cosT = cos(theta);
      const float sinT1 = sin(theta + step);
      const float cosT1 = cos(theta + step);

      buf->add_quad(// Vertices
         make_vector(R * cosT1, 0.1f, R * sinT1),
         make_vector(R * cosT1, 0.0f, R * sinT1),
         make_vector(R * cosT, 0.0f, R * sinT),
         make_vector(R * cosT, 0.1f, R * sinT),

         // Normals
         make_vector(cosT1, 0.0f, sinT1),
         make_vector(cosT1, 0.0f, sinT1),
         make_vector(cosT, 0.0f, sinT),
         make_vector(cosT, 0.0f, sinT),

         METAL);
   }

   // Inner edge
   for (float theta = 0; theta < M_PI / 2.0f; theta += step) {
      const float sinT = sin(theta);
      const float cosT = cos(theta);
      const float sinT1 = sin(theta + step);
      const float cosT1 = cos(theta + step);

      buf->add_quad(// Vertices
         make_vector(r * cosT, 0.1f, r * sinT),
         make_vector(r * cosT, 0.0f, r * sinT),
         make_vector(r * cosT1, 0.0f, r * sinT1),
         make_vector(r * cosT1, 0.1f, r * sinT1),

         // Normals
         make_vector(-cosT, 0.0f, -sinT),
         make_vector(-cosT, 0.0f, -sinT),
         make_vector(-cosT1, 0.0f, -sinT1),
         make_vector(-cosT1, 0.0f, -sinT1),

         METAL);
   }
}

void CurvedTrackHelper::merge_curved_rail(IMeshBufferPtr buf, int base_radius,
   Vector<float> off, float y_angle)
{
   IMeshBufferPtr rail_buf;

   CurvedRailMeshMap::iterator it = curved_rail_meshes.find(base_radius);
   if (it != curved_rail_meshes.end())
      rail_buf = (*it).second;
   else {
      rail_buf = make_mesh_buffer();

      generate_curved_rail_mesh(rail_buf, base_radius, INNER_RAIL);
      generate_curved_rail_mesh(rail_buf, base_radius, OUTER_RAIL);

      curved_rail_meshes[base_radius] = rail_buf;
   }

   buf->merge(rail_buf, off, y_angle);
}

void CurvedTrackHelper::merge_curved_track(IMeshBufferPtr buf, Vector<float> off,
   int base_radius, track::Angle start_angle, track::Angle end_angle) const
{
   transform_to_origin(off, base_radius, start_angle);

   merge_curved_rail(buf, base_radius, off, static_cast<float>(start_angle));

   const float length =
      deg_to_rad(static_cast<float>(end_angle - start_angle)) * base_radius;
   const int num_sleepers = static_cast<int>(length * SLEEPERS_PER_UNIT);
   const float sleeper_angle =
      static_cast<float>(end_angle - start_angle) / num_sleepers;

   for (int i = 0; i < num_sleepers; i++) {

      float y_angle = static_cast<float>(start_angle) + (i + 0.5f)*sleeper_angle;
      Vector<float> t =
         make_vector(0.0f, 0.0f, static_cast<float>(base_radius) - 0.5f);

      merge_sleeper(buf, off + rotateY(t, y_angle), y_angle);
   }
}

float track::flat_gradient_func(const TravelToken& t, float d)
{
   return 0.0f;
}
