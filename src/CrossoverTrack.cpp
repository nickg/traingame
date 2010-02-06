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

#include "ITrackSegment.hpp"
#include "TrackCommon.hpp"
#include "XMLBuilder.hpp"

#include <stdexcept>
#include <cassert>

#include <GL/gl.h>
#include <GL/glu.h>
#include <boost/lexical_cast.hpp>

using namespace std;
using namespace std::tr1;
using namespace std::tr1::placeholders;
using namespace boost;

// A section of track that allows travelling along both axis
class CrossoverTrack : public ITrackSegment,
                       public enable_shared_from_this<CrossoverTrack> {
public:
    CrossoverTrack() : myX(0), myY(0) {}
    ~CrossoverTrack() {}

    void setOrigin(int x, int y) { myX = x; myY = y; }
   
    void render() const;
    double segmentLength(const track::TravelToken& aToken) const;
    bool isValidDirection(const track::Direction& aDirection) const;
    track::Connection nextPosition(const track::TravelToken& aToken) const;
    void getEndpoints(std::list<Point<int> >& aList) const;
    ITrackSegmentPtr mergeExit(const Point<int>& aPoint,
	const track::Direction& aDirection);
    track::TravelToken getTravelToken(track::Position aPosition,
	track::Direction aDirection) const;
    xml::element toXml() const;
    void nextState() {}
    void prevState() {}
    bool hasMultipleStates() const { return false; }
    
private:
    void transform(const track::TravelToken& aToken, double aDelta) const;
   
    int myX, myY;
};

void CrossoverTrack::render() const
{
    // Render the y-going rails and sleepers
    glPushMatrix();

    renderStraightRail();
   
    glRotated(90.0, 0.0, 1.0, 0.0);
    glTranslated(-0.4, 0.0, 0.0);

    for (int i = 0; i < 4; i++) {
	renderSleeper();
	glTranslated(0.25, 0.0, 0.0);
    }
   
    glPopMatrix();

    // Render the x-going rails and sleepers
    glPushMatrix();
   
    glRotated(90.0, 0.0, 1.0, 0.0);

    renderStraightRail();
   
    glRotated(90.0, 0.0, 1.0, 0.0);
    glTranslated(-0.4, 0.0, 0.0);

    for (int i = 0; i < 4; i++) {
	renderSleeper();
	glTranslated(0.25, 0.0, 0.0);
    }

    glPopMatrix();
}

double CrossoverTrack::segmentLength(const track::TravelToken& aToken) const
{
    return 1.0;
}

track::TravelToken
CrossoverTrack::getTravelToken(track::Position aPosition,
    track::Direction aDirection) const
{
    if (!isValidDirection(aDirection))
	throw runtime_error
	    ("Invalid direction on crossover: " + lexical_cast<string>(aDirection));

    track::TravelToken tok = {
	aDirection,
	aPosition,
	bind(&CrossoverTrack::transform, this, _1, _2)
    };
    return tok;
}

void CrossoverTrack::transform(const track::TravelToken& aToken,
    double aDelta) const
{
    assert(aDelta < 1.0);
   
    bool backwards = aToken.direction== -axis::X || aToken.direction == -axis::Y;

    if (backwards) {
	aDelta = 1.0 - aDelta;
    }

    track::Direction dir = backwards ? -aToken.direction : aToken.direction;

    const double xTrans = dir == axis::X ? aDelta : 0;
    const double yTrans = dir == axis::Y ? aDelta : 0;

    glTranslated(static_cast<double>(myX) + xTrans,
	0.0,
	static_cast<double>(myY) + yTrans);

    if (dir == axis::Y)
	glRotated(-90.0, 0.0, 1.0, 0.0);

    glTranslated(-0.5, 0.0, 0.0);
   
    if (backwards)
	glRotated(-180.0, 0.0, 1.0, 0.0);
}

bool CrossoverTrack::isValidDirection(const track::Direction& aDirection) const
{
    return aDirection == axis::X || aDirection == axis::Y
	|| aDirection == -axis::Y || aDirection == -axis::X;
}

track::Connection
CrossoverTrack::nextPosition(const track::TravelToken& aToken) const
{
    if (aToken.direction == axis::X)
	return make_pair(makePoint(myX + 1, myY), axis::X);
    else if (aToken.direction == -axis::X)
	return make_pair(makePoint(myX - 1, myY), -axis::X);
    else if (aToken.direction == axis::Y)
	return make_pair(makePoint(myX, myY + 1), axis::Y);
    else if (aToken.direction == -axis::Y)
	return make_pair(makePoint(myX, myY - 1), -axis::Y);
    else
	throw runtime_error
	    ("Invalid direction on crossover: " + lexical_cast<string>(aToken.direction));
}

void CrossoverTrack::getEndpoints(std::list<Point<int> >& aList) const
{
    aList.push_back(makePoint(myX, myY));
}

ITrackSegmentPtr CrossoverTrack::mergeExit(const Point<int>& aPoint,
    const track::Direction& aDirection)
{
    if (aPoint == makePoint(myX, myY)
	&& isValidDirection(aDirection))
	return shared_from_this();

    // No way to extend a crossover
    return ITrackSegmentPtr();
}

xml::element CrossoverTrack::toXml() const
{
    return xml::element("crossoverTrack");
}

ITrackSegmentPtr makeCrossoverTrack()
{
    return ITrackSegmentPtr(new CrossoverTrack);
}
