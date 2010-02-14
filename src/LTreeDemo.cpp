//
//  Copyright (C) 2010  Nick Gasson
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

#include "IScreen.hpp"
#include "ILogger.hpp"
#include "IScenery.hpp"

#include <GL/gl.h>

class LTreeDemo : public IScreen {
public:
    LTreeDemo();

    // IScreen interface
    void display(IGraphicsPtr context) const;
    void overlay() const;
    void update(IPickBufferPtr aPickBuffer, int aDelta);
    void onKeyDown(SDLKey aKey);
    void onKeyUp(SDLKey aKey) {}
    void onMouseMove(IPickBufferPtr aPickBuffer, int x, int y,
        int xrel, int yrel) {}
    void onMouseClick(IPickBufferPtr pick_buffer, int x, int y,
        MouseButton button);
    void onMouseRelease(IPickBufferPtr pick_buffer, int x, int y,
        MouseButton button) {}
   
private:
    ISceneryPtr ltree;
    float rotation;
};

LTreeDemo::LTreeDemo()
    : rotation(0.0f)
{
    ltree = makeLTree();
}

void LTreeDemo::display(IGraphicsPtr context) const
{
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);

    glTranslatef(0.0f, -1.0f, -5.0f);
    glRotatef(rotation, 0.0f, 1.0f, 0.0f);

    ltree->render();
}

void LTreeDemo::update(IPickBufferPtr aPickBuffer, int aDelta)
{
    rotation += static_cast<float>(aDelta) * 0.1f;
}

void LTreeDemo::overlay() const
{
   
}

void LTreeDemo::onMouseClick(IPickBufferPtr pick_buffer, int x, int y,
    MouseButton button)
{
   
}

void LTreeDemo::onKeyDown(SDLKey aKey)
{
    switch (aKey) {
    case SDLK_r:
        ltree = makeLTree();
        break;
    default:
        break;
    }
}

IScreenPtr makeLTreeDemo()
{
    return IScreenPtr(new LTreeDemo);
}
