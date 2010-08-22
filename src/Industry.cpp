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

#include "IIndustry.hpp"

class Industry : public IIndustry {
public:
   Industry(CargoType produces, CargoType consumes);

   // IIndustry interface
   CargoType produces() const { return produces_; }
   CargoType consumes() const { return consumes_; }

private:
   CargoType produces_, consumes_;
};

Industry::Industry(CargoType produces, CargoType consumes)
   : produces_(produces), consumes_(consumes)
{

}

IIndustryPtr make_industry(CargoType produces, CargoType consumes)
{
   return IIndustryPtr(new Industry(produces, consumes));
}


