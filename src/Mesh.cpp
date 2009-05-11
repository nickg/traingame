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

#include <vector>

#include <GL/gl.h>
#include <boost/cast.hpp>

using namespace std;
using namespace boost;

// Concrete implementation of mesh buffers
struct MeshBuffer : IMeshBuffer {
   MeshBuffer();
   ~MeshBuffer() {}

   void add(const Vertex& aVertex, const Normal& aNormal);

   static MeshBuffer* get(IMeshBufferPtr aPtr)
   {
      return polymorphic_cast<MeshBuffer*>(aPtr.get());
   }
   
   vector<Vertex> vertices;
   vector<Normal> normals;
   vector<Index> indices;
};

MeshBuffer::MeshBuffer()
{

}

void MeshBuffer::add(const Vertex& aVertex, const Normal& aNormal)
{
   const int index = vertices.size();
   vertices.push_back(aVertex);
   normals.push_back(aNormal);
   indices.push_back(index);
}

// Simple implementation using display lists
class DisplayListMesh : public IMesh {
public:
   DisplayListMesh(IMeshBufferPtr aBuffer);
   ~DisplayListMesh();

   void render() const;
private:
   IMeshBufferPtr myBuffer;
};

DisplayListMesh::DisplayListMesh(IMeshBufferPtr aBuffer)
   : myBuffer(aBuffer)
{

}

DisplayListMesh::~DisplayListMesh()
{

}

void DisplayListMesh::render() const
{
   const MeshBuffer* buf = MeshBuffer::get(myBuffer);
   
   glPushAttrib(GL_ENABLE_BIT);

   glEnable(GL_COLOR_MATERIAL);
   glDisable(GL_BLEND);
   glDisable(GL_TEXTURE_2D);
   glEnable(GL_CULL_FACE);

   glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
   
   glBegin(GL_TRIANGLES);

   vector<MeshBuffer::Index>::const_iterator it;
   for (it = buf->indices.begin();
        it != buf->indices.end(); ++it) {

      const MeshBuffer::Normal& n = buf->normals[*it];
      glNormal3f(n.x, n.y, n.z);
      
      const MeshBuffer::Vertex& v = buf->vertices[*it];
      glVertex3f(v.x, v.y, v.z);      
   }        
           
   glEnd();

   glPopAttrib();
}

IMeshPtr makeMesh(IMeshBufferPtr aBuffer)
{
   return IMeshPtr(new DisplayListMesh(aBuffer));
}

IMeshBufferPtr makeMeshBuffer()
{
   return IMeshBufferPtr(new MeshBuffer);
}
