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
#pragma once

#include <cstdlib>

class Coordinate {
	private:
		int _x, _y;
	public:
		Coordinate(int valuex = 0,int = 0);
		int x() const;
		int x(int);
		int y() const;
		int y(int);
		bool operator<(const Coordinate) const;
		bool operator==(const Coordinate) const;
		bool operator!=(const Coordinate) const;
		Coordinate operator+(int);
		Coordinate operator-(int);
		Coordinate operator+(Coordinate);
};

class PathCoordinate : public Coordinate {
	private:
		int _moveCost, _distance;
		Coordinate _parent;
	public:
		PathCoordinate(int = 0,int = 0,int = 0,int = 0, Coordinate = Coordinate(0,0));
		int moveCost() const;
		void moveCost(int);
		int distance() const;
		void distance(int);
		Coordinate parent() const;
		void parent(Coordinate);
		bool operator<(const PathCoordinate) const;
		bool operator==(const PathCoordinate) const;
};
