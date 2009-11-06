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

#include "IScenery.hpp"
#include "IBillboard.hpp"
#include "Random.hpp"

#include <boost/random.hpp>

class Tree : public IScenery {
public:
   Tree();

   // IScenery interface
   void render() const;
   void setPosition(float x, float y, float z);
   
private:
   IBillboardPtr billboard;
   int texHeight;
};

Tree::Tree()
{
   const char* trees[] = {
      "data/images/a-tree.png",
      "data/images/a-nother-tree.png",
      "data/images/cloud-tree.png"
   };

   static UniformInt rnd(0, 2);

   billboard = makeCylindricalBillboard(
      loadTexture(trees[rnd()]));
   billboard->setScale(1.2f);
}

void Tree::render() const
{
   billboard->render();
}

void Tree::setPosition(float x, float y, float z)
{
   billboard->setPosition(x, y + 0.6f, z);
}

ISceneryPtr makeTree()
{
   return ISceneryPtr(new Tree);
}
