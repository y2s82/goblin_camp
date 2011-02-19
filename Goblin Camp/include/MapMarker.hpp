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

#include <boost/serialization/serialization.hpp>
#include <libtcod.hpp>

#include "Coordinate.hpp"

enum MarkerType {
	FLASHINGMARKER
};

class MapMarker {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	MarkerType type;
	TCODColor origColor, color;
	int duration;
	int graphic;
	int x, y;
	float counter;
public:
	MapMarker(MarkerType=FLASHINGMARKER, int graphic='?', Coordinate position=Coordinate(0,0), 
		int duration=1, TCODColor color=TCODColor::pink);
	bool Update();
	int X() const;
	int Y() const;
	int Graphic() const;
	TCODColor Color() const;
};

BOOST_CLASS_VERSION(MapMarker, 0)

template<class Archive>
void MapMarker::save(Archive & ar, const unsigned int version) const {
	ar & type;
	ar & origColor.r;
	ar & origColor.g;
	ar & origColor.b;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & duration;
	ar & graphic;
	ar & x;
	ar & y;
	ar & counter;
}

template<class Archive>
void MapMarker::load(Archive & ar, const unsigned int version) {
	ar & type;
	ar & origColor.r;
	ar & origColor.g;
	ar & origColor.b;
	ar & color.r;
	ar & color.g;
	ar & color.b;
	ar & duration;
	ar & graphic;
	ar & x;
	ar & y;
	ar & counter;
}
