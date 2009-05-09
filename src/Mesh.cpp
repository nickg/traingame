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

#include "IMesh.hpp"

#include <GL/gl.h>

using namespace std;

// Simple implementation using display lists
class DisplayListMesh : public IMesh {
public:
   DisplayListMesh(const MeshBuffer& aBuffer);
   ~DisplayListMesh();

   void render() const;
private:
   const MeshBuffer myBuffer;
};

DisplayListMesh::DisplayListMesh(const MeshBuffer& aBuffer)
   : myBuffer(aBuffer)
{

}

DisplayListMesh::~DisplayListMesh()
{

}

void DisplayListMesh::render() const
{
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_COLOR_MATERIAL);
   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
   
   glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
   
   glBegin(GL_TRIANGLES);

   vector<MeshBuffer::Index>::const_iterator it;
   for (it = myBuffer.indices.begin();
        it != myBuffer.indices.end(); ++it) {

      const MeshBuffer::Vertex& v = myBuffer.vertices[*it];
      glVertex3f(v.x, v.y, v.z);
      
   }        
           
   glEnd();

   glPopAttrib();
}

IMeshPtr makeMesh(const MeshBuffer& aBuffer)
{
   return IMeshPtr(new DisplayListMesh(aBuffer));
}
