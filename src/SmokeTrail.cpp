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
#include <random>
#include <cstdlib>

using namespace std;

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
   
   // A single smoke particle
   struct Particle {
      float x, y, z;
      float scale;
      float r, g, b, a;
   };
   
private:
   void newParticle();
   bool moveParticle(Particle& aParticle, int aDelta);
   
   mutable list<Particle> myParticles;  // Need to sort particles in render()
   float myX, myY, myZ;
   IBillboardPtr myBillboard;

   // New particles are created every `mySpawnDelay`
   int mySpawnDelay, mySpawnCounter;
};

SmokeTrail::SmokeTrail()
   : myX(0.0f), myY(0.0f), myZ(0.0f),
     mySpawnDelay(500), mySpawnCounter(0)
{
   ITexturePtr particle(loadTexture("data/images/smoke_particle.png"));
   myBillboard = makeSphericalBillboard(particle);
}

// Returns true if the particle is dead
bool SmokeTrail::moveParticle(Particle& aParticle, int aDelta)
{
   const float ySpeed = 0.2f;
   const float growth = 0.2f;
   const float decay = 0.2f;

   const float time = static_cast<float>(aDelta) / 1000.0f;
   
   aParticle.y += ySpeed * time;
   aParticle.scale += growth * time;

   // Kill the particle if it becomes invisible
   return (aParticle.a -= decay * time) <= 0.0f;
}

void SmokeTrail::update(int aDelta)
{
   // Move the existing particles
   for (list<Particle>::iterator it = myParticles.begin();
        it != myParticles.end(); ++it)
      if (moveParticle(*it, aDelta))
         it = myParticles.erase(it);
   
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
   static variate_generator<mt19937, normal_distribution<> >
      colourRand(mt19937(time(NULL)), normal_distribution<>(0.0f, 0.05f));

   const float col = 0.8f + colourRand();
   
   Particle p = {
      myX, myY, myZ,  // Position
      0.2f,           // Scale
      col, col, col,  // Colour
      0.9f,           // Alpha
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

ISmokeTrailPtr makeSmokeTrail()
{
   return ISmokeTrailPtr(new SmokeTrail);
}
