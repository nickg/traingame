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

#include <ctime>

#include <boost/lexical_cast.hpp>
#include <boost/random.hpp>

// Concrete implementation of stations
class Station : public IStation {
public:
   Station();
   ~Station() {}

   // IStation interface
   const string& name() const { return myName; }
   void setName(const string& aName) { myName = aName; }
   HighlightColour highlightColour() const { return myColour; }
private:
   string myName;
   HighlightColour myColour;
};

Station::Station()
{
   using namespace boost;
   
   // Generate a unique station name;
   static int nameCounter = 1;
   myName = "Station" + lexical_cast<string>(nameCounter++);

   // Generate a random colour
   static variate_generator<mt19937, uniform_real<float> >
      colourRand(mt19937(static_cast<uint32_t>(time(NULL))), 
                 uniform_real<float>(0.2f, 1.0f));
   myColour = make_tuple(colourRand(),
                         colourRand(),
                         colourRand());
}

IStationPtr makeStation()
{
   return IStationPtr(new Station);
}
