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

#ifndef INC_OPENGL_HELPER_HPP
#define INC_OPENGL_HELPER_HPP

#include "Platform.hpp"
#include "IWindow.hpp"
#include "IGraphics.hpp"
#include "IPickBuffer.hpp"

// Helper functions used by the different IWindow implementations
void initGL();
void drawGLScene(IWindowPtr aWindow, IGraphicsPtr aContext, IScreenPtr aScreen);
void resizeGLScene(IWindowPtr aWindow);

// Wrappers for OpenGL picking features
void beginPick(IWindowPtr aWindow, unsigned* aBuffer, int x, int y);
unsigned endPick(unsigned* aBuffer);

const float NEAR_CLIP = 0.1f;
const float FAR_CLIP = 70.0f;

// Standard implementation of IGraphics
class OpenGLGraphics : public IGraphics {
public:
   OpenGLGraphics();
   ~OpenGLGraphics();
   
   bool cuboidInViewFrustum(float x, float y, float z,
                            float sizeX, float sizeY, float sizeZ);
   bool cubeInViewFrustum(float x, float y, float z, float size);
   bool pointInViewFrustum(float x, float y, float z);
   void setCamera(const Vector<float>& aPos,
                  const Vector<float>& aRotation);
   void lookAt(const Vector<float> anEyePoint,
               const Vector<float> aTargetPoint);
private:
   Frustum myViewFrustum;
};

#endif
