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

#ifndef INC_IIMAGE_HPP
#define INC_IIMAGE_HPP

#include <tr1/memory>
#include <string>

namespace gui {

   // Displays a simple 2D image
   // This is not a control but is used by some of them
   struct IImage {
      virtual ~IImage() {}

      virtual void render(int x, int y) const = 0;
      virtual int width() const = 0;
      virtual int height() const = 0;
   };

   typedef std::tr1::shared_ptr<IImage> IImagePtr;

   IImagePtr makeImage(const std::string& aFile);
}

#endif
