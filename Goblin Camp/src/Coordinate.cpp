/* Copyright 2010 Ilkka Halila
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

#include "Coordinate.hpp"

Coordinate::Coordinate(int valuex, int valuey) { _x = valuex; _y = valuey; }

int Coordinate::x() const { return _x;}
int Coordinate::x(int value) { _x = value; return _x; }
int Coordinate::y() const { return _y;}
int Coordinate::y(int value) { _y = value; return _y; }

bool Coordinate::operator<(Coordinate other) const {
	if (_x != other.x()) return _x < other.x();
	else return _y < other.y();
}

bool Coordinate::operator==(Coordinate other) const {
	return (_x == other.x() && _y == other.y());
}

bool Coordinate::operator!=(Coordinate other) const {
	return (_x != other.x() || _y != other.y());
}

Coordinate Coordinate::operator+(int other) {
	return Coordinate(_x + other, _y + other);
}

Coordinate Coordinate::operator-(int other) {
	return Coordinate(_x - other, _y - other);
}

Coordinate Coordinate::operator+(Coordinate other) {
    return Coordinate(_x + other.x(), _y + other.y());
}

template<class Archive>
void Coordinate::save(Archive & ar, unsigned int version) const {
	ar & _x;
	ar & _y;
}

template<class Archive>
void Coordinate::load(Archive & ar, unsigned int version) {
	ar & _x;
	ar & _y;
}
