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

#ifndef INC_ISKYBOX_HPP
#define INC_ISKYBOX_HPP

#include "Platform.hpp"

// Simple textured skybox
struct ISkyBox {
   virtual ~ISkyBox() {}

   virtual void apply(float an_angle=0.0f) const = 0;
};

typedef shared_ptr<ISkyBox> ISkyBoxPtr;

ISkyBoxPtr make_sky_box(const string& a_base_name);

#endif
