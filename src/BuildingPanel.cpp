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

#include "BuildingPanel.hpp"

using namespace gui;

BuildingPanel::BuildingPanel()
{
   myPanel = makePanel("Select Building", makeFlowBox(FLOW_BOX_VERT));
   myPanel->setOrigin(50, 50);
}

BuildingPanel::~BuildingPanel()
{

}

void BuildingPanel::render() const
{
   myPanel->render();
}

