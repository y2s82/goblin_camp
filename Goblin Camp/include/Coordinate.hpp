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
