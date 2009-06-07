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

#include <list>
#include <cstdlib>
#include <ctime>

#include <boost/random.hpp>

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
   };
   
private:
   void newParticle();
   bool moveParticle(Particle& aParticle, int aDelta);
   
   mutable list<Particle> myParticles;  // Need to sort particles in render()
   float myX, myY, myZ;
   IBillboardPtr myBillboard;

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
   ITexturePtr particle(loadTexture("data/images/smoke_particle.png"));
   myBillboard = makeSphericalBillboard(particle);
}

// Returns true if the particle is dead
bool SmokeTrail::moveParticle(Particle& aParticle, int aDelta)
{
   const float ySpeed = 0.4f;
   const float growth = 0.3f;
   const float decay = 0.3f;
   const float appear = 4.0f;
   const float slowdown = 0.1f;
   const float xWind = 0.02f;
   const float zWind = 0.01f;
   
   const float time = static_cast<float>(aDelta) / 1000.0f;
   
   aParticle.x += aParticle.xv + (xWind * time);
   aParticle.y += aParticle.yv + (ySpeed * time);
   aParticle.z += aParticle.zv + (zWind * time);

   aParticle.xv = max(aParticle.xv - (slowdown * time), 0.0f);
   aParticle.yv = max(aParticle.yv - (slowdown * time), 0.0f);
   aParticle.zv = max(aParticle.zv - (slowdown * time), 0.0f);   
   
   aParticle.scale += growth * time;

   const float maxA = 0.8f;
   if (aParticle.appearing) {
      if ((aParticle.a += appear * time) >= maxA) {
         aParticle.a = maxA;
         aParticle.appearing = false;
      }
      return false;
   }
   else {
      // Kill the particle if it becomes invisible
      return (aParticle.a -= decay * time) <= 0.0f;
   }
}

void SmokeTrail::update(int aDelta)
{
   // Move the existing particles
   list<Particle>::iterator it = myParticles.begin();
   while (it != myParticles.end()) {
      if (moveParticle(*it, aDelta))
         it = myParticles.erase(it);
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
   using namespace boost;

   // Random number generator for colour variance
   static variate_generator<mt19937, normal_distribution<float> >
      colourRand(mt19937(static_cast<uint32_t>(time(NULL))), 
                 normal_distribution<float>(0.0f, 0.06f));

   // Random number generator for position variance
   static variate_generator<mt19937, normal_distribution<float> >
      posRand(mt19937(static_cast<uint32_t>(time(NULL))), 
              normal_distribution<float>(0.0f, 0.07f));

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
   };
   
   myParticles.push_back(p);
}

struct CmpDistanceToCam {
   bool operator()(const SmokeTrail::Particle& lhs,
                   const SmokeTrail::Particle& rhs)
   {
      return distanceToCamera(makeVector(lhs.x, lhs.y, lhs.z))
         > distanceToCamera(makeVector(rhs.x, rhs.y, rhs.z));
   }
};

void SmokeTrail::render() const
{
   myParticles.sort(CmpDistanceToCam());
   
   for (list<Particle>::const_iterator it = myParticles.begin();
        it != myParticles.end(); ++it) {
      myBillboard->setPosition((*it).x, (*it).y, (*it).z);
      myBillboard->setColour((*it).r, (*it).g, (*it).b, (*it).a);
      myBillboard->setScale((*it).scale);
      myBillboard->render();
   }
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
   myYSpeed = y;
   myZSpeed = z;
}

ISmokeTrailPtr makeSmokeTrail()
{
   return ISmokeTrailPtr(new SmokeTrail);
}
