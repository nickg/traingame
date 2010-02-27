//
//  Copyright (C) 2009  Nick Gasson
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
   void setPosition(float x, float y, float z);
   void update(int aDelta);
   void setDelay(int aDelay) { mySpawnDelay = aDelay; }
   void setVelocity(float x, float y, float z);
   
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
   void newParticle();
   bool moveParticle(Particle& aParticle, int aDelta);
   
   list<Particle> particles;
   float myX, myY, myZ;

   ITexturePtr particleTex;

   // New particles are created every `mySpawnDelay`
   int mySpawnDelay, mySpawnCounter;

   // Velocity at which the emitter is moving
   float myXSpeed, myYSpeed, myZSpeed;
};

SmokeTrail::SmokeTrail()
   : myX(0.0f), myY(0.0f), myZ(0.0f),
     mySpawnDelay(500), mySpawnCounter(0),
     myXSpeed(0.0f), myYSpeed(0.0f), myZSpeed(0.0f)
{
   particleTex = loadTexture("images/smoke_particle.png");
}

// Returns true if the particle is dead
bool SmokeTrail::moveParticle(Particle& p, int aDelta)
{
   const float ySpeed = 0.4f;
   const float growth = 0.3f;
   const float decay = 0.3f;
   const float appear = 4.0f;
   const float slowdown = 0.1f;
   const float xWind = 0.02f;
   const float zWind = 0.01f;
   
   const float time = static_cast<float>(aDelta) / 1000.0f;
   
   p.x += p.xv + (xWind * time);
   p.y += p.yv + (ySpeed * time);
   p.z += p.zv + (zWind * time);

   p.xv = max(p.xv - (slowdown * time), 0.0f);
   p.yv = max(p.yv - (slowdown * time), 0.0f);
   p.zv = max(p.zv - (slowdown * time), 0.0f);   
   
   p.scale += growth * time;

   p.billboard->setPosition(p.x, p.y, p.z);
   p.billboard->setColour(p.r, p.g, p.b, p.a);
   p.billboard->setScale(p.scale);
   
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

void SmokeTrail::update(int aDelta)
{
   // Move the existing particles
   list<Particle>::iterator it = particles.begin();
   while (it != particles.end()) {
      if (moveParticle(*it, aDelta))
         it = particles.erase(it);
      else
         ++it;
   }
   
   mySpawnCounter -= aDelta;

   if (mySpawnCounter <= 0) {
      // Generate a new particle
      newParticle();

      mySpawnCounter = mySpawnDelay;
   }
}

void SmokeTrail::newParticle()
{
   // Random number generator for colour variance
   static Normal<float> colourRand(0.0f, 0.06f);

   // Random number generator for position variance
   static Normal<float> posRand(0.0f, 0.07f);

   const float col = 0.7f + colourRand();

   const float dx = posRand();
   const float dz = posRand();
   
   Particle p = {
      myX + dx, myY, myZ + dz,      // Position
      myXSpeed, myYSpeed, myZSpeed, // Speed
      0.4f,                         // Scale
      col, col, col,                // Colour
      0.0f,                         // Alpha
      true,                         // Appearing

      makeSphericalBillboard(particleTex)
   };

   p.billboard->setPosition(p.x, p.y, p.z);
   p.billboard->setColour(p.r, p.g, p.b, p.a);
   p.billboard->setScale(p.scale);
   
   particles.push_back(p);
}

void SmokeTrail::render() const
{   
   for (list<Particle>::const_iterator it = particles.begin();
        it != particles.end(); ++it)
      (*it).billboard->render();
}

void SmokeTrail::setPosition(float x, float y, float z)
{
   myX = x;
   myY = y;
   myZ = z;
}

void SmokeTrail::setVelocity(float x, float y, float z)
{   
   myXSpeed = x;
   myYSpeed = y + 0.02f;  // Make smoke shoot up
   myZSpeed = z;
}

ISmokeTrailPtr makeSmokeTrail()
{
   return ISmokeTrailPtr(new SmokeTrail);
}
