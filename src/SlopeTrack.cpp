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
#include "IXMLSerialisable.hpp"
#include "XMLBuilder.hpp"
#include "BezierCurve.hpp"

#include <cassert>

// Like StraightTrack but with a change of height
class SlopeTrack : public ITrackSegment {
public:
   SlopeTrack(track::Direction axis, Vector<float> slope,
      Vector<float> slopeBefore, Vector<float> slopeAfter);

   // ITrackSegment interface
   void render() const;
   void setOrigin(int x, int y) { origin = makePoint(x, y); }
   double segmentLength(const track::TravelToken& token) const;
   track::TravelToken getTravelToken(track::Position pos,
      track::Direction dir) const;
   bool isValidDirection(const track::Direction& dir) const;
   track::Connection nextPosition(const track::TravelToken& token) const;
   void getEndpoints(vector<Point<int> >& output) const;
   void getCovers(vector<Point<int> >& output) const {};
   ITrackSegmentPtr mergeExit(Point<int> where, track::Direction dir);
   
   bool hasMultipleStates() const { return false; }
   void nextState() {}
   void prevState() {}
   void setStateRenderHint() {}

   // IXMLSerialisable inteface
   xml::element toXml() const;

private:
   Point<int> origin;
   BezierCurve<float> curve;
};

SlopeTrack::SlopeTrack(track::Direction axis, Vector<float> slope,
   Vector<float> slopeBefore, Vector<float> slopeAfter)
{
   Vector<float> p1, p2, p3, p4;

   if (axis == axis::X) {
      p1 = makeVector(0.0f, 0.0f, 0.0f);
      p2 = makeVector(0.1f, 0.0f, 0.0f);
      p3 = makeVector(0.9f, slope.y, 0.0f);
      p4 = makeVector(1.0f, slope.y, 0.0f);
   }
   else {
      p1 = makeVector(0.0f, 0.0f, 0.0f);
      p2 = makeVector(0.0f, 0.0f, 0.1f);
      p3 = makeVector(0.0f, slope.y, 0.9f);
      p4 = makeVector(0.0f, slope.y, 1.0f);
   }
}

void SlopeTrack::render() const
{
   
}

double SlopeTrack::segmentLength(const track::TravelToken& token) const
{
   assert(false);
   return 1.0;  // TODO: use Pythagoras
}

bool SlopeTrack::isValidDirection(const track::Direction& dir) const
{
   assert(false);
   return false;  // TODO
}

track::Connection SlopeTrack::nextPosition(
   const track::TravelToken& token) const
{
   // TODO
   assert(false);
}

track::TravelToken SlopeTrack::getTravelToken(track::Position pos,
      track::Direction dir) const
{
   // TODO
   assert(false);
}

void SlopeTrack::getEndpoints(vector<Point<int> >& output) const
{

}

ITrackSegmentPtr SlopeTrack::mergeExit(Point<int> where, track::Direction dir)
{
   return ITrackSegmentPtr();
}

xml::element SlopeTrack::toXml() const
{
   return xml::element("slopeTrack");
}

ITrackSegmentPtr makeSlopeTrack(track::Direction axis, Vector<float> slope,
   Vector<float> slopeBefore, Vector<float> slopeAfter)
{
   return ITrackSegmentPtr(
      new SlopeTrack(axis, slopeBefore, slope, slopeAfter));
}
