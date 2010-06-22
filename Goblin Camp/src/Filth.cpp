#include "Filth.hpp"
#include "Game.hpp"

FilthNode::FilthNode(int nx, int ny, int ndep) : x(nx), y(ny), depth(ndep)
{
}

FilthNode::~FilthNode() {}

void FilthNode::Update() {
}

void FilthNode::Draw(Coordinate upleft, TCODConsole* console) {
	int screenX = x - upleft.x();
	int screenY = y - upleft.y();

	if (depth > 0) {
		if (screenX >= 0 && screenX < console->getWidth() &&
			screenY >= 0 && screenY < console->getHeight()) {
			console->putCharEx(screenX, screenY, (depth < 5) ? '~' : '#', TCODColor::darkerOrange, TCODColor::black);
		}
	}
}

int FilthNode::Depth() {return depth;}
void FilthNode::Depth(int val) {depth=val;}
Coordinate FilthNode::Position() {return Coordinate(x,y);}
