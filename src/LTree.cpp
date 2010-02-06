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

    enum Token {
	X = 'X',   // Only used for replacement
	F = 'F',   // Draw forward
	L = '-',   // Turn left N degrees
	R = '+',   // Turn right N degrees
	B = '[',   // Push position
	E = ']',   // Pop position
    };

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

    // IScenery interface
    void render() const;
    void setPosition(float x, float y, float z);
private:
    struct RenderState {
	RenderState()
	{
	    widthStack.push(2.0f);
	}

	stack<float> widthStack;
    };
   
    void interpret(Token t, RenderState& rs) const;

    LSystem ls;
    Vector<float> position;
   
    static const Rule rules[];
};

const Rule LTree::rules[] = {
    Rule(X, "F-[[X]+X]+F[+FX]-X"),
    Rule(X, "F-[[X]+X]+FF[+FX]-X"),
    Rule(X, "F+[[X]-X]-F[-FX]+X"),
    Rule(X, "+[[X]-X]-F[-FX]+X"),
    Rule(F, "FF"),
};

LTree::LTree()
    : ls(rules, sizeof(rules) / sizeof(Rule), X)
{
    const int N_GENERATIONS = 5;
   
    //debug() << "Initial: " << ls;
    for (int n = 0; n < N_GENERATIONS; n++) {
	evolve(ls);
	//debug() << "n=" << n << ": " << ls;
    }
}

void LTree::interpret(Token t, RenderState& rs) const
{
    const float SEGMENT_LEN = 0.025f;
    const float LEAF_LEN = 0.1f;
   
    switch (t) {
    case 'F':
	glLineWidth(rs.widthStack.top());
	glBegin(GL_LINES);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.0f, SEGMENT_LEN, 0.0f);
	glEnd();
	glTranslatef(0.0f, SEGMENT_LEN, 0.0f);
	break;
    case 'X':
	/*glPushAttrib(GL_CURRENT_BIT);
	  glColor3f(0.0f, 0.8f, 0.0f);
	  glBegin(GL_TRIANGLES);
	  glVertex3f(0.0f, 0.0f, 0.0f);
	  glVertex3f(LEAF_LEN, 0.0f, 0.0f);
	  glVertex3f(0.0f, -LEAF_LEN, LEAF_LEN);
	  glEnd();
	  glPopAttrib();*/
	break;
    case '-':
	glRotatef(25.0, 0.0f, 0.0f, 1.0f);
	glRotatef(25.0, 0.0f, 1.0f, 0.0f);
	break;
    case '+':
	glRotatef(-25.0, 0.0f, 0.0f, 1.0f);
	glRotatef(-25.0, 0.0f, 1.0f, 0.0f);
	break;
    case '[':
	rs.widthStack.push(rs.widthStack.top() * 0.9f);
	glPushMatrix();
	break;
    case ']':
	rs.widthStack.pop();
	glPopMatrix();
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
    using namespace placeholders;

    glPushMatrix();

    gl::translate(position);
   
    glColor3f(0.0f, 0.0f, 0.0f);
   
    RenderState rs;
    for_each(ls.state.begin(), ls.state.end(),
	bind(&LTree::interpret, this, _1, rs));

    glPopMatrix();
}

void LTree::setPosition(float x, float y, float z)
{
    position.x = x;
    position.y = y;
    position.z = z;
}

ISceneryPtr makeLTree()
{
    return ISceneryPtr(new LTree);
}

