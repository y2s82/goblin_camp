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
#include "stdafx.hpp"

#include <cmath>
#include "Coordinate.hpp"

Coordinate::Coordinate(int valuex, int valuey) { x = valuex; y = valuey; }

int Coordinate::X() const { return x;}
int Coordinate::X(int value) { x = value; return x; }
int Coordinate::Y() const { return y;}
int Coordinate::Y(int value) { y = value; return y; }

bool Coordinate::operator<(Coordinate other) const {
	if (x != other.X()) return x < other.X();
	else return y < other.Y();
}

bool Coordinate::operator==(Coordinate other) const {
	return (x == other.X() && y == other.Y());
}

bool Coordinate::operator!=(Coordinate other) const {
	return (x != other.X() || y != other.Y());
}

Coordinate Coordinate::operator+(int other) {
	return Coordinate(x + other, y + other);
}

Coordinate Coordinate::operator-(int other) {
	return Coordinate(x - other, y - other);
}

Coordinate Coordinate::operator+(Coordinate other) {
	return Coordinate(x + other.X(), y + other.Y());
}

template<class Archive>
void Coordinate::save(Archive & ar, unsigned int version) const {
	ar & x;
	ar & y;
}

template<class Archive>
void Coordinate::load(Archive & ar, unsigned int version) {
	ar & x;
	ar & y;
}

int Distance(int x0, int y0, int x1, int y1) {
	return (abs(y1 - y0) + abs(x1 - x0));
}

int Distance(Coordinate a, Coordinate b) {
	return Distance(a.X(), a.Y(), b.X(), b.Y());
}
