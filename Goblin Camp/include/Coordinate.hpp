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
#include <cstdlib> // int abs(int)

enum Direction {
	NORTH,
	NORTHEAST,
	EAST,
	SOUTHEAST,
	SOUTH,
	SOUTHWEST,
	WEST,
	NORTHWEST,
	NODIRECTION
};

class Coordinate {
	friend class boost::serialization::access;
	friend int Distance(const Coordinate&, const Coordinate&);
	friend std::size_t hash_value(const Coordinate&);
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()
	
	int x, y;
public:
	Coordinate(int x = 0, int y = 0) : x(x), y(y) { }
	
	static Coordinate DirectionToCoordinate(Direction dir) {
		static boost::array<Coordinate,9> coordsToDirs = {Coordinate(0,-1), Coordinate(1,-1), Coordinate(1,0), Coordinate(1,1), Coordinate(0,1), Coordinate(-1,1), Coordinate(-1,0), Coordinate(-1,1), Coordinate(0,0)};
		return coordsToDirs[dir];
	}

	inline int X() const {
		return x;
	}
	
	inline int X(const int& v) {
		return x = v;
	}
	
	inline int Y() const {
		return y;
	}
	
	inline int Y(const int& v) {
		return y = v;
	}
	
	inline bool operator<(const Coordinate& other) const {
		if (x != other.x) return x < other.x;
		else return y < other.y;
	}
	
	inline bool operator>(const Coordinate& other) const {
		return !(operator<(other));
	}
	
	inline bool operator==(const Coordinate& other) const {
		return (x == other.x && y == other.y);
	}
	
	inline bool operator!=(const Coordinate& other) const {
		return !(operator==(other));
	}
	
	inline Coordinate operator+(const int& scalar) const {
		return Coordinate(x + scalar, y + scalar);
	}
	
	inline Coordinate operator-(const int& scalar) const {
		return Coordinate(x - scalar, y - scalar);
	}
	
	inline Coordinate operator+(const Coordinate& other) const {
		return Coordinate(x + other.x, y + other.y);
	}
	
	inline Coordinate operator-(const Coordinate& other) const {
		return Coordinate(x - other.x, y - other.y);
	}
};

inline int Distance(const int& x0, const int& y0, const int& x1, const int& y1) {
	return (abs(y1 - y0) + abs(x1 - x0));
}

inline int Distance(const Coordinate& a, const Coordinate& b) {
	return Distance(a.x, a.y, b.x, b.y);
}
