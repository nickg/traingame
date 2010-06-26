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

#include "IStation.hpp"
#include "Random.hpp"

#include <boost/lexical_cast.hpp>

// Concrete implementation of stations
class Station : public IStation {
public:
   Station();
   ~Station() {}

   // IStation interface
   const string& name() const { return my_name; }
   void set_name(const string& a_name) { my_name = a_name; }
   int id() const { return my_id; }
   void set_id(int an_id) { my_id = an_id; }
   Colour highlight_colour() const { return colour; }
   bool highlight_visible() const { return is_highlight_visible; }
   void set_highlight_visible(bool on_off);
private:
   string my_name;
   Colour colour;
   bool is_highlight_visible;
   int my_id;
};

Station::Station()
   : is_highlight_visible(false)
{
   using namespace boost;
   
   // Generate a unique station name;
   static int name_counter = 1;
   my_id = name_counter++;
   my_name = "Station" + lexical_cast<string>(my_id);

   // Generate a random colour
   static Uniform<float> colour_rand(0.3f, 1.0f);
   colour = make_colour(colour_rand(), colour_rand(), colour_rand());
}

void Station::set_highlight_visible(bool on_off)
{
   is_highlight_visible = on_off;
}

IStationPtr make_station()
{
   return IStationPtr(new Station);
}
