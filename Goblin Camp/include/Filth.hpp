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
#pragma once

#include <boost/enable_shared_from_this.hpp>
#include <libtcod.hpp>

#include "data/Serialization.hpp"
#include "Coordinate.hpp"

class FilthNode : public boost::enable_shared_from_this<FilthNode> {
	GC_SERIALIZABLE_CLASS
	
	Coordinate pos;
	int depth;
	int graphic;
	TCODColor color;
public:
	FilthNode(const Coordinate& pos = zero, int depth = 0);
	~FilthNode();

	void Update();
	int Depth();
	void Depth(int);
	Coordinate Position();
	int GetGraphic();
	TCODColor GetColor();
};

BOOST_CLASS_VERSION(FilthNode, 0)
