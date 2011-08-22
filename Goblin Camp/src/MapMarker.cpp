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

#include <boost/math/constants/constants.hpp>

#include "MapMarker.hpp"
#include "Random.hpp"
#include "Coordinate.hpp"

MapMarker::MapMarker(MarkerType t, int g, Coordinate pos, int d, TCODColor c) : type(t), 
	origColor(c), color(c), duration(d), graphic(g),
x(pos.X()), y(pos.Y()), counter(0.0f) {
}

bool MapMarker::Update() {
	if (duration > 0) --duration;
	color = TCODColor::lerp(origColor, TCODColor::white, std::abs(std::sin(counter)));
	counter += 0.1f;
	if (counter > boost::math::constants::pi<float>()) counter = 0.0f;
	return duration != 0;
}

int MapMarker::X() const {
	return x;
}

int MapMarker::Y() const {
	return y;
}

Coordinate MapMarker::Position() const {
	return Coordinate(x,y);
}

int MapMarker::Graphic() const {
	return graphic;
}

TCODColor MapMarker::Color() const {
	return color;
}

void MapMarker::save(OutputArchive& ar, const unsigned int version) const {
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

void MapMarker::load(InputArchive& ar, const unsigned int version) {
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
