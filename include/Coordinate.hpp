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

#include <boost/array.hpp>
#include <cstdlib> // int abs(int)

#include "data/Serialization.hpp"

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
	GC_SERIALIZABLE_CLASS
	
	friend int Distance(const Coordinate&, const Coordinate&);
	friend std::size_t hash_value(const Coordinate&);
	
	int x, y;

public:
	Coordinate(int x = 0, int y = 0) : x(x), y(y) { }

	/* beware, those are pairwise {min,max} : Coordinate::min(p,q) is *not* equivalent to (p < q ? p : q)
	   
	   Typical use: (min(p,q), max(p,q)) are the (low, high) corner of the bounding box rectangle of p and q
	 */
	static Coordinate min(const Coordinate& p, const Coordinate &q) {
		return Coordinate(std::min(p.x, q.x), std::min(p.y,q.y));
	}
	static Coordinate max(const Coordinate& p, const Coordinate &q) {
		return Coordinate(std::max(p.x, q.x), std::max(p.y,q.y));
	}

	static Coordinate DirectionToCoordinate(Direction dir) {
		static boost::array<Coordinate,9> coordsToDirs = 
		{ // gcc complains unless there are two level of braces
		  // see http://stackoverflow.com/questions/2687701/question-on-boost-array-initializer
			{Coordinate(0,-1),  // North
			 Coordinate(1,-1),  // North-east
			 Coordinate(1,0),   // East
			 Coordinate(1,1),   // South-east
			 Coordinate(0,1),   // South
			 Coordinate(-1,1),  // South-west
			 Coordinate(-1,0),  // West
			 Coordinate(-1,-1), // North-west
			 Coordinate(0,0)}   // No direction
		};
		return coordsToDirs[dir];
	}

	/* specific and generic accessors
	   
	   Coordinate provides both specific accessors p.X(), p.Y() and
	   operator[] accessors p[0], p[1]. Both are read/write.

	   Specific accessors are slightly more type-safe -- operator[]
	   accesses such as top[3] are not ruled out -- so should be
	   preferred when we statically know which dimension to access.
	   
	   The generic accessors are designed to be used for
	   dimension-generic code, that is code which access both X and
	   Y in the same way. For example:

		   randomLocation.X(Random::Generate(upperCorner.X(), lowerCorner.X()));
		   randomLocation.Y(Random::Generate(upperCorner.Y(), lowerCorner.Y()));

	   can be profitably turned into:

		   for (int d = 0; d < 2; ++d)
			   randomLocation[d] =  Random::Generate(upperCorner[d], lowerCorder[d]);

	   This latter form will avoid all kinds of copy-paste bugs.
	*/
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

	// in order not to have to handle failure, we use a bool-like
	// semantics where 0 gets mapped to X and everyone else to Y
	inline int operator[](int d) const {
		if (d == 0) return x;
		else return y;
	}
	inline int &operator[](int d) {
		if (d == 0) return x;
		else return y;
	}

	//useful for legacy TCOD interaction
	inline int *Xptr() {
		return &x;
	}
	inline int *Yptr() {
		return &y;
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

	inline Coordinate operator*(const int& scalar) const {
		return Coordinate(x * scalar, y * scalar);
	}

	inline Coordinate operator/(const int& scalar) const {
		return Coordinate(x / scalar, y / scalar);
	}

	inline Coordinate& operator+=(const Coordinate& other) {
		x += other.x;
		y += other.y;
		return *this;
	}
	inline Coordinate& operator+=(const int& scalar) {
		x += scalar;
		y += scalar;
		return *this;
	}

	/* The inside, onEdges and shrink functions are declined in two varieties:

	   - the "rectangle" version takes a couple of corners (low, high) (with the
		 coordinates of (low) all smaller than those of (high)), which the set
		 of points whose coordinates are between (low,high), with (high)
		 *included* in the rectangle.

	   - the "extent" version takes an origin and an extent, that is a couple
		 (width, height) of *positive* integers, which delimits the dimension of
		 the rectangle. The (origin+extent) point is *excluded* from the
		 rectangle.

	   One may convert between both representations: the extent (origin,extent)
	   is the rectangle (low=origin, high=origin+extent-1), and conversely the
	   rectangle (low,high) has extent (low,high-low+1).

	   Both representations are convenient in different situations.
	 */

	inline bool insideRectangle(const Coordinate& low, const Coordinate& high) const {
		return (   x >= low.X() && x <= high.X()
		        && y >= low.Y() && y <= high.Y());
	}

	//are we inside the rectangle starting at origin and with extent (width,height) excluded?
	inline bool insideExtent(const Coordinate& origin, const Coordinate& extent) const {
		return insideRectangle(origin, origin+extent-1);
	}

	inline bool onRectangleEdges(const Coordinate& low, const Coordinate& high) const {
		return (x == low.X() || x == high.X() || y == low.Y() || y == high.Y());
	}
	inline bool onExtentEdges(const Coordinate& origin, const Coordinate& extent) const {
		return onRectangleEdges(origin, origin + extent - 1);
	}

	inline Coordinate shrinkRectangle(const Coordinate& low, const Coordinate& high) const {
		Coordinate res(x,y);
		for (int d = 0; d < 2; ++d)
			res[d] = std::max(low[d], std::min(high[d], res[d]));
		return res;
	}

	inline Coordinate shrinkExtent(const Coordinate& origin, const Coordinate& extent) const {
		return shrinkRectangle(origin, origin + extent - 1);
	};
};

inline int Distance(const Coordinate& p, const Coordinate& q) {
	int distance = 0;
	//dim-genericity may here seem a bit overkill, but will be nice if
	//we were to change to Euclidian distance for example
	for (int d = 0; d < 2; ++d)
		distance += abs(q[d] - p[d]);
	return distance;
}

inline int Distance(const int& x0, const int& y0, const int& x1, const int& y1) {
	//note: this reimplementation is not terribly efficient, but will go away soon anyway
	return Distance(Coordinate(x0, y0), Coordinate(x1, y1));
}

static const Coordinate zero(0, 0);
static const Coordinate undefined(-1, -1);

BOOST_CLASS_VERSION(Coordinate, 0)
