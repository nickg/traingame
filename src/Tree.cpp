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

#include "IScenery.hpp"
#include "IModel.hpp"
#include "OpenGLHelper.hpp"
#include "IResource.hpp"
#include "IXMLParser.hpp"
#include "ResourceCache.hpp"
#include "ILogger.hpp"
#include "XMLBuilder.hpp"
#include "Random.hpp"

#include <stdexcept>

#include <boost/cast.hpp>

// A tree which is just a 3D model
class Tree : public IScenery, public IXMLCallback {
public:
   Tree(IResourcePtr res);

   // IScenery interface
   void render() const;
   void set_position(float x, float y, float z);
   void set_angle(float a) { angle = a; }
   const string& name() const { return name_; }
   void merge(IMeshBufferPtr buf);
   Point<int> size() const;

   // IXMLCallback interface
   void text(const string& local_name, const string& content);

   // IXMLSerialisable interface
   xml::element to_xml() const;
   
private:
   Vector<float> position;
   IModelPtr model;
   float angle;
   string name_;

   struct ParserState {
      string model_file;
      float scale;
      IResourcePtr res;
   } *parser_state;
};

Tree::Tree(IResourcePtr res)
{
   static IXMLParserPtr parser = makeXMLParser("schemas/tree.xsd");

   parser_state = new ParserState;
   parser_state->res = res;
   
   parser->parse(res->xml_file_name(), *this);

   model = load_model(res, parser_state->model_file, parser_state->scale);
   
   delete parser_state;
}

void Tree::text(const string& local_name, const string& content)
{
   if (local_name == "model")
      parser_state->model_file = content;
   else if (local_name == "scale")
      parser_state->scale = boost::lexical_cast<float>(content);
   else if (local_name == "name") {
      const string& expected_name = parser_state->res->name();
      if (content != expected_name)
         throw runtime_error(
            "Expected tree name to be '" + expected_name
            + "' but found'" + content + "' in XML");
      else
         name_ = content;
   }
}

Point<int> Tree::size() const
{
   return make_point(1, 1);
}

void Tree::set_position(float x, float y, float z)
{
   position = make_vector(x, y, z);
}

void Tree::render() const
{
   glPushMatrix();

   gl::translate(position);
   glRotatef(angle, 0.0f, 1.0f, 0.0f);
   model->render();
   
   glPopMatrix();
}

void Tree::merge(IMeshBufferPtr buf)
{
   model->merge(buf, position, angle);
}

xml::element Tree::to_xml() const
{
   return xml::element("tree")
      .add_attribute("angle", angle)
      .add_attribute("name", name_);
}

namespace {
   Tree* load_tree_xml(IResourcePtr res)
   {
      log() << "Loading tree from " << res->xml_file_name();
   
      return new Tree(res);
   }

   shared_ptr<Tree> load_tree_fromCache(const string& name)
   {
      static ResourceCache<Tree> cache(load_tree_xml, "trees");
      return cache.load_copy(name);
   }
}

ISceneryPtr load_tree(const string& name)
{
   shared_ptr<Tree> tree = load_tree_fromCache(name);
   
   // Randomise the new tree's angle
   static Uniform<float> angle_rand(0.0f, 360.0f);
   tree.get()->set_angle(angle_rand());

   return ISceneryPtr(tree);
}

ISceneryPtr load_tree(const AttributeSet& attrs)
{
   // Unserialise a tree
   float angle;
   string name;
   attrs.get("name", name);
   attrs.get("angle", angle);

   shared_ptr<Tree> tree = load_tree_fromCache(name);
   tree->set_angle(angle);

   return ISceneryPtr(tree);
}
