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

PathCoordinate::PathCoordinate(int valuex, int valuey, int valuemc, int valuedistance, Coordinate vparent) : Coordinate(valuex, valuey) {
	_moveCost = valuemc;
	_distance = valuedistance;
	_parent = vparent;
}

int PathCoordinate::moveCost() const { return _moveCost;}
void PathCoordinate::moveCost(int value) { _moveCost = value; }
int PathCoordinate::distance() const { return _distance;}
void PathCoordinate::distance(int value) { _distance = value; }
Coordinate PathCoordinate::parent() const { return _parent; }
void PathCoordinate::parent(Coordinate value) { _parent = value; }


//TODO: Bad hack to ensure uniqueness
bool PathCoordinate::operator<(PathCoordinate other) const {
	return ((x() + y()*500) < (other.x() + other.y()*500));
}

bool PathCoordinate::operator==(PathCoordinate other) const {
	return (x() == other.x() && y() == other.y());
}
