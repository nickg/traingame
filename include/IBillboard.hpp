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

#ifndef INC_IBILLBOARD_HPP
#define INC_IBILLBOARD_HPP

#include "Platform.hpp"
#include "ITexture.hpp"
#include "Maths.hpp"

// Generic quad billboard with a single texture
struct IBillboard {
   virtual ~IBillboard() {}

   virtual void render() const = 0;

   virtual void setPosition(float x, float y, float z) = 0;
   virtual void setScale(float aScale) = 0;
   virtual void setColour(float r, float g, float b, float a) = 0;
};

typedef std::tr1::shared_ptr<IBillboard> IBillboardPtr;

IBillboardPtr makeCylindricalBillboard(ITexturePtr aTexture);
IBillboardPtr makeSphericalBillboard(ITexturePtr aTexture);

// This should be called once per frame to render all billboards
// in the correct orientation
void setBillboardCameraOrigin(Vector<float> aPosition);

// Billboards normally need to be depth sorted
// This calculates the distance of a point to the camera
float distanceToCamera(Vector<float> aPosition);

// Draw all billboards saved during this frame
void renderBillboards();

#endif
