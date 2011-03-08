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

#include "Blood.hpp"
#include "Coordinate.hpp"

BloodNode::BloodNode(int nx, int ny, int ndep) : x(nx), y(ny), depth(ndep)
{
}

BloodNode::~BloodNode() {}

void BloodNode::Update() {
}

void BloodNode::Draw(Coordinate upleft, TCODConsole* console) {
}

int BloodNode::Depth() {return depth;}
void BloodNode::Depth(int val) {depth=val;}
Coordinate BloodNode::Position() {return Coordinate(x,y);}

void BloodNode::save(OutputArchive& ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}

void BloodNode::load(InputArchive& ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}
