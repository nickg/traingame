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

#include "ITrackSegment.hpp"
#include "TrackCommon.hpp"
#include "ILogger.hpp"
#include "XMLBuilder.hpp"
#include "BezierCurve.hpp"

#include <cassert>

// Spline curves which start and finish in the same direction
class SBend : public ITrackSegment {
public:
   SBend(track::Direction dir, int xoff, int yoff);

   // ITrackSegment interface
   void render() const;
   void setOrigin(int x, int y, float h) { x_ = x; y_ = y; height = h; }
   float segmentLength(const track::TravelToken& token) const;
   bool isValidDirection(const track::Direction& dir) const;
   track::Connection nextPosition(const track::TravelToken& token) const;
   void getEndpoints(vector<Point<int> >& output) const;
   void getCovers(vector<Point<int> >& output) const;
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   track::TravelToken getTravelToken(track::Position pos,
      track::Direction dir) const;
   void nextState() {}
   void prevState() {}
   bool hasMultipleStates() const { return false; }
   void setStateRenderHint() {}

   // IXMLSerialisable interface
   xml::element toXml() const;

private:
   void transform(const track::TravelToken& token, float delta) const;
   void ensureValidDirection(track::Direction dir) const;

   int x_, y_;
   int xOffset, yOffset;
   float height;
};

SBend::SBend(track::Direction dir, int xoff, int yoff)
   : x_(0), y_(0), xOffset(xoff), yOffset(yoff), height(0.0f)
{

}

void SBend::render() const
{

}

float SBend::segmentLength(const track::TravelToken& token) const
{
   assert(false);
}

bool SBend::isValidDirection(const track::Direction& dir) const
{
   assert(false);
}

track::Connection SBend::nextPosition(const track::TravelToken& token) const
{
   assert(false);
}

void SBend::getEndpoints(vector<Point<int> >& output) const
{
   assert(false);
}

void SBend::getCovers(vector<Point<int> >& output) const
{
   assert(false);
}

ITrackSegmentPtr SBend::mergeExit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

track::TravelToken SBend::getTravelToken(track::Position pos,
   track::Direction dir) const
{
   assert(false);
}

void SBend::transform(const track::TravelToken& token, float delta) const
{
   assert(false);
}

void SBend::ensureValidDirection(track::Direction dir) const
{
   assert(false);
}

xml::element SBend::toXml() const
{
   assert(false);
}

ITrackSegmentPtr makeSBend(track::Direction dir, int xoff, int yoff)
{
   return ITrackSegmentPtr(new SBend(dir, xoff, yoff));
}
