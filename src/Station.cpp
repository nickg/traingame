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
   const string& name() const { return myName; }
   void setName(const string& aName) { myName = aName; }
   int id() const { return myId; }
   void setId(int anId) { myId = anId; }
   Colour highlightColour() const { return colour; }
   bool highlightVisible() const { return isHighlightVisible; }
   void setHighlightVisible(bool onOff);
private:
   string myName;
   Colour colour;
   bool isHighlightVisible;
   int myId;
};

Station::Station()
   : isHighlightVisible(false)
{
   using namespace boost;
   
   // Generate a unique station name;
   static int nameCounter = 1;
   myId = nameCounter++;
   myName = "Station" + lexical_cast<string>(myId);

   // Generate a random colour
   static Uniform<float> colourRand(0.3f, 1.0f);
   colour = makeColour(colourRand(), colourRand(), colourRand());
}

void Station::setHighlightVisible(bool onOff)
{
   isHighlightVisible = onOff;
}

IStationPtr makeStation()
{
   return IStationPtr(new Station);
}
