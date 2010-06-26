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

#include "ISmokeTrail.hpp"
#include "IBillboard.hpp"
#include "Random.hpp"

#include <list>

// Concrete implementation of smoke trails
class SmokeTrail : public ISmokeTrail {
public:
   SmokeTrail();
   ~SmokeTrail() {}

   // ISmokeTrail interface
   void render() const;
   void set_position(float x, float y, float z);
   void update(int a_delta);
   void set_delay(int a_delay) { my_spawn_delay = a_delay; }
   void set_velocity(float x, float y, float z);
   
   // A single smoke particle
   struct Particle {
      float x, y, z;
      float xv, yv, zv;
      float scale;
      float r, g, b, a;
      bool appearing;

      IBillboardPtr billboard;
   };
   
private:
   void new_particle();
   bool move_particle(Particle& a_particle, int a_delta);
   
   list<Particle> particles;
   float myX, myY, myZ;

   ITexturePtr particle_tex;

   // New particles are created every `my_spawn_delay`
   int my_spawn_delay, my_spawn_counter;

   // Velocity at which the emitter is moving
   float myXSpeed, myYSpeed, myZSpeed;
};

SmokeTrail::SmokeTrail()
   : myX(0.0f), myY(0.0f), myZ(0.0f),
     my_spawn_delay(500), my_spawn_counter(0),
     myXSpeed(0.0f), myYSpeed(0.0f), myZSpeed(0.0f)
{
   particle_tex = load_texture("images/smoke_particle.png");
}

// Returns true if the particle is dead
bool SmokeTrail::move_particle(Particle& p, int a_delta)
{
   const float y_speed = 0.4f;
   const float growth = 0.3f;
   const float decay = 0.3f;
   const float appear = 4.0f;
   const float slowdown = 0.1f;
   const float x_wind = 0.02f;
   const float z_wind = 0.01f;
   
   const float time = static_cast<float>(a_delta) / 1000.0f;
   
   p.x += p.xv + (x_wind * time);
   p.y += p.yv + (y_speed * time);
   p.z += p.zv + (z_wind * time);

   p.xv = max(p.xv - (slowdown * time), 0.0f);
   p.yv = max(p.yv - (slowdown * time), 0.0f);
   p.zv = max(p.zv - (slowdown * time), 0.0f);   
   
   p.scale += growth * time;

   p.billboard->set_position(p.x, p.y, p.z);
   p.billboard->set_colour(p.r, p.g, p.b, p.a);
   p.billboard->set_scale(p.scale);
   
   const float maxA = 0.8f;
   if (p.appearing) {
      if ((p.a += appear * time) >= maxA) {
         p.a = maxA;
         p.appearing = false;
      }
      return false;
   }
   else {
      // Kill the particle if it becomes invisible
      return (p.a -= decay * time) <= 0.0f;
   }
}

void SmokeTrail::update(int a_delta)
{
   // Move the existing particles
   list<Particle>::iterator it = particles.begin();
   while (it != particles.end()) {
      if (move_particle(*it, a_delta))
         it = particles.erase(it);
      else
         ++it;
   }
   
   my_spawn_counter -= a_delta;

   if (my_spawn_counter <= 0) {
      // Generate a new particle
      new_particle();

      my_spawn_counter = my_spawn_delay;
   }
}

void SmokeTrail::new_particle()
{
   // Random number generator for colour variance
   static Normal<float> colour_rand(0.0f, 0.06f);

   // Random number generator for position variance
   static Normal<float> pos_rand(0.0f, 0.07f);

   const float col = 0.7f + colour_rand();

   const float dx = pos_rand();
   const float dz = pos_rand();
   
   Particle p = {
      myX + dx, myY, myZ + dz,      // Position
      myXSpeed, myYSpeed, myZSpeed, // Speed
      0.4f,                         // Scale
      col, col, col,                // Colour
      0.0f,                         // Alpha
      true,                         // Appearing

      make_spherical_billboard(particle_tex)
   };

   p.billboard->set_position(p.x, p.y, p.z);
   p.billboard->set_colour(p.r, p.g, p.b, p.a);
   p.billboard->set_scale(p.scale);
   
   particles.push_back(p);
}

void SmokeTrail::render() const
{   
   for (list<Particle>::const_iterator it = particles.begin();
        it != particles.end(); ++it)
      (*it).billboard->render();
}

void SmokeTrail::set_position(float x, float y, float z)
{
   myX = x;
   myY = y;
   myZ = z;
}

void SmokeTrail::set_velocity(float x, float y, float z)
{   
   myXSpeed = x;
   myYSpeed = y + 0.02f;  // Make smoke shoot up
   myZSpeed = z;
}

ISmokeTrailPtr make_smoke_trail()
{
   return ISmokeTrailPtr(new SmokeTrail);
}
