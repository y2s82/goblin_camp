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

#include <boost/serialization/split_member.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <libtcod.hpp>

class Coordinate;

class BloodNode : public boost::enable_shared_from_this<BloodNode> {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	int x, y;
	int depth;
	int graphic;
	TCODColor color;
public:
	BloodNode(int x=0, int y=0, int depth=0);
	~BloodNode();

	void Update();
	void Draw(Coordinate, TCODConsole*);
	int Depth();
	void Depth(int);
	Coordinate Position();
};

#include <boost/serialization/version.hpp>
BOOST_CLASS_VERSION(BloodNode, 0)

template<class Archive>
void BloodNode::save(Archive & ar, const unsigned int version) const {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}

template<class Archive>
void BloodNode::load(Archive & ar, const unsigned int version) {
	ar & x;
	ar & y;
	ar & depth;
	ar & graphic;
	ar & color.r;
	ar & color.g;
	ar & color.b;
}
