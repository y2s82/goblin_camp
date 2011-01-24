/* Copyright 2010-2011 Ilkka Halila
This file is part of Goblin Camp.

Goblin Camp is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Goblin Camp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License 
along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.*/
#include "stdafx.hpp"

#include "Random.hpp"
#include "Filth.hpp"
#include "Game.hpp"

FilthNode::FilthNode(int nx, int ny, int ndep) : x(nx), y(ny)
{
	color.b = 0;
	Depth(ndep);
}

FilthNode::~FilthNode() {}

void FilthNode::Update() {
}

int FilthNode::GetGraphic()
{
	return (depth < 5 ) ? '~' : '#';
}

TCODColor FilthNode::GetColor()
{
	return color;
}

int FilthNode::Depth() {return depth;}
void FilthNode::Depth(int val) {
	depth=val;
	color.r = 170 - std::min(Map::Inst()->GetCorruption(x, y), 40) + Random::Generate(55);
	color.g = 80 - std::min(Map::Inst()->GetCorruption(x, y), 80) + Random::Generate(60);
}
Coordinate FilthNode::Position() {return Coordinate(x,y);}
