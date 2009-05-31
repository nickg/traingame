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

using namespace std;

// Concrete implementation of smoke trails
class SmokeTrail : public ISmokeTrail {
public:
   SmokeTrail();
   ~SmokeTrail() {}

   // ISmokeTrail interface
   void render() const;
   void setPosition(float x, float y, float z);
   
private:
   float myX, myY, myZ;
   IBillboardPtr myBillboard;
};

SmokeTrail::SmokeTrail()
   : myX(0.0f), myY(0.0f), myZ(0.0f)
{
   ITexturePtr particle(loadTexture("data/images/smoke_particle.png"));
   myBillboard = makeSphericalBillboard(particle);
}

void SmokeTrail::render() const
{
   
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
