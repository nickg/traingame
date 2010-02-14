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
#include "ILogger.hpp"
#include "Random.hpp"
#include "OpenGLHelper.hpp"
#include "Matrix.hpp"

#include <list>
#include <vector>
#include <ostream>
#include <iterator>
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <stack>

#include <GL/gl.h>

namespace lsystem {

    typedef char Token;

    typedef list<Token> TokenList;

    struct Rule {
	Rule(Token lhs, const TokenList& rhs)
	    : lhs(lhs), rhs(rhs)
	{}

	Rule(Token lhs, const char* tokenStr);

	Token lhs;
	TokenList rhs;
    };

    struct LSystem {
	LSystem(const Rule* rules, int nRules, Token start);

	const Rule* rules;
	int nRules;
	Token start;
	TokenList state;
    };

    void evolve(LSystem& l);      

    ostream& operator<<(ostream& os, const LSystem& l)
    {
	copy(l.state.begin(), l.state.end(),
	    ostream_iterator<char>(os));
	return os;
    }
}

lsystem::Rule::Rule(Token lhs, const char* tokenStr)
    : lhs(lhs)
{
    while (*tokenStr)
	rhs.push_back((Token)*tokenStr++);
}

lsystem::LSystem::LSystem(const Rule* rules, int nRules, Token start)
    : rules(rules), nRules(nRules), start(start)
{
    state.push_back(start);
}

void lsystem::evolve(LSystem& l)
{
    static UniformInt rnd(0, 1000);
   
    TokenList::iterator it = l.state.begin();
    do {
	vector<const Rule*> applicable;
      
	for (int r = 0; r < l.nRules; r++) {
	    if (l.rules[r].lhs == *it) {
		//l.state.insert(it, l.rules[r].rhs.begin(),
		//   l.rules[r].rhs.end());
		//replaced = true;
		//break;
		applicable.push_back(&l.rules[r]);
	    }
	}

	if (applicable.empty())
	    ++it;
	else {
	    int chosen = rnd() % applicable.size();
         
	    l.state.insert(it, applicable[chosen]->rhs.begin(),
		applicable[chosen]->rhs.end());
            
	    it = l.state.erase(it);
	}
            
    } while (it != l.state.end());
}

using namespace lsystem;

// Trees generated by L-systems
class LTree : public IScenery {
public:
    LTree();
    ~LTree();

    // IScenery interface
    void render() const;
    void setPosition(float x, float y, float z);
private:
    struct RenderState {
	RenderState()
	{
	    widthStack.push(5.5f);
	    mStack.push(Matrix<float, 4>::identity());
	    lengthStack.push(0.1f);
	}

	stack<float> widthStack, lengthStack;
	stack<Matrix<float, 4> > mStack;
    };
   
    void interpret(Token t, RenderState& rs) const;
    void renderToDisplayList() const;
    int countLines() const;

    LSystem ls;
    Vector<float> position;

    GLuint displayList;
   
    static const Rule rules[];
};

const Rule LTree::rules[] = {
    //Rule('S', "FFA"),
    //Rule('A', "[&B]>[&B]>[&FB]>FA"),
    //Rule('B', "F[L]~[&'B]'B"),
    //Rule('L', "~lfL"),
    Rule('S', "X"),
    Rule('X', "F<[[X]+X]>F[+FX]>&X"),
    //Rule('X', "F<[[X]+X]>F[+FX]+X"),
    Rule('X', "F<[+FX]>F[[X]+X]>&X"),
    //Rule('X', "[&X]>[&FX]"),
    //Rule('X', "+[[X]-X]-F[-FX]+X"),
    Rule('F', "FF"),
};

LTree::LTree()
    : ls(rules, sizeof(rules) / sizeof(Rule), 'S')
{
    const int N_GENERATIONS = 5;
   
    //debug() << "Initial: " << ls;
    for (int n = 0; n < N_GENERATIONS; n++) {
	evolve(ls);
	//debug() << "n=" << n << ": " << ls;
    }

    debug() << ls;

    displayList = glGenLists(1);
    renderToDisplayList();

    debug() << "LTree has " << countLines() << " lines";
}

LTree::~LTree()
{
    glDeleteLists(displayList, 1);
}

void LTree::interpret(Token t, RenderState& rs) const
{
    const float LEAF_LEN = 0.2f;
    const float LEAF_WIDTH = 0.06f;
    
    Matrix<float, 4>& m = rs.mStack.top();
   
    float l = rs.lengthStack.top();
	    
    switch (t) {
    case 'f':
	l /= 10.0f;
    case 'F':
	{
	    Vector<float> v1 = m.transform(makeVector(0.0f, 0.0f, 0.0f));
	    Vector<float> v2 = m.transform(makeVector(0.0f, l, 0.0f));
	    Vector<float> n = m.transform(makeVector(0.0f, 1.0f, 0.0f));
	    n.normalise();

	    gl::colour(makeRGB(159, 71, 17));
	    glLineWidth(rs.widthStack.top());
	    glBegin(GL_LINES);
	    {
		gl::normal(n);
		gl::vertex(v1);
		gl::vertex(v2);
	    }
	    glEnd();
	    
	    m *= Matrix<float, 4>::translation(0.0f, l, 0.0f);
	}
	break;
    case 'B':
    case 'l':
	glPushAttrib(GL_CURRENT_BIT);
	gl::colour(makeRGB(0, 83, 0, 0.9f));
	glDisable(GL_CULL_FACE);
	glBegin(GL_QUADS);
	{
	    const Vector<float> top = m.transform(makeVector(0.0f, 0.0f, 0.0f));
	    gl::vertex(top);
	    gl::vertex(top + makeVector(-LEAF_WIDTH/2, -LEAF_LEN/3, 0.0f));
	    gl::vertex(top + makeVector(0.0f, -LEAF_LEN, 0.0f));
	    gl::vertex(top + makeVector(LEAF_WIDTH/2, -LEAF_LEN/3, 0.0f));
	}
	glEnd();
	glPopAttrib();
	break;
    case 'A':
    case 'L':
    case 'X':
	break;
    case '-':
	m *= Matrix<float, 4>::rotation(25.0f, 0, 0, 1);
	//m *= Matrix<float, 4>::rotation(25.0f, 0, 1, 0);
	break;
    case '+':	
	m *= Matrix<float, 4>::rotation(-25.0f, 0, 0, 1);
	//m *= Matrix<float, 4>::rotation(-25.0f, 0, 1, 0);
	break;
    case '&':
	// Pitch down
	m *= Matrix<float, 4>::rotation(-25.0f, 0, 0, 1);
	break;
    case '^':
	// Pitch up
	m *= Matrix<float, 4>::rotation(25.0f, 0, 0, 1);
	break;
    case '<':
	// Roll left
	m *= Matrix<float, 4>::rotation(-100.0f, 0, 1, 0);
	break;
    case '>':
	// Roll right
	m *= Matrix<float, 4>::rotation(100.0f, 0, 1, 0);
	break;
    case '~':
	// Turn, pitch, and roll
	m *= Matrix<float, 4>::rotation(15.0f, 1, 0, 0);
	m *= Matrix<float, 4>::rotation(15.0f, 0, 1, 0);
	m *= Matrix<float, 4>::rotation(15.0f, 0, 0, 1);
	break;
    case '\'':
	// Decrease length
	rs.lengthStack.top() *= 0.5f;
	break;
    case '[':
	rs.widthStack.push(rs.widthStack.top() * 0.8f);
	rs.lengthStack.push(rs.lengthStack.top());
	rs.mStack.push(m);
	break;
    case ']':
	rs.widthStack.pop();
	rs.mStack.pop();
	rs.lengthStack.pop();
	break;
    default:
	{
	    ostringstream ss;
	    ss << "Bad token in LTree: " << static_cast<char>(t);
	    throw runtime_error(ss.str());
	}
    }
}

void LTree::render() const
{
    glPushAttrib(GL_ENABLE_BIT | GL_LINE_BIT);

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glPushMatrix();
    
    gl::translate(position);
    glCallList(displayList);

    glPopMatrix();
    
    glPopAttrib();
}

void LTree::renderToDisplayList() const
{
    using namespace placeholders;

    glNewList(displayList, GL_COMPILE);

    glPushMatrix();

    gl::colour(makeRGB(159, 71, 17));
   
    RenderState rs;
    for_each(ls.state.begin(), ls.state.end(),
	bind(&LTree::interpret, this, _1, rs));

    glPopMatrix();

    glEndList();
}

void LTree::setPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
}

int LTree::countLines() const
{
    int count = 0;
    for (TokenList::const_iterator it = ls.state.begin();
	 it != ls.state.end(); ++it)
	if (*it == 'F')
	    ++count;
    
    return count;
}	

ISceneryPtr makeLTree()
{
    return ISceneryPtr(new LTree);
}


