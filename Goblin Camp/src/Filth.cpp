#include "Filth.hpp"
#include "Game.hpp"

FilthNode::FilthNode(int nx, int ny, int ndep) : x(nx), y(ny), depth(ndep)
{
}

FilthNode::~FilthNode() {}

void FilthNode::Update() {
}

void FilthNode::Draw(Coordinate center) {
    int screenX = x - center.x() + Game::Inst()->ScreenWidth()/2;
	int screenY = y - center.y() + Game::Inst()->ScreenHeight()/2;

	if (depth > 0) {
		if (screenX >= 0 && screenX < Game::Inst()->ScreenWidth() &&
			screenY >= 0 && screenY < Game::Inst()->ScreenHeight()) {
			TCODConsole::root->putCharEx(screenX, screenY, (depth < 5) ? '~' : '#', TCODColor::darkerOrange, TCODColor::black);
		}
	}
}

int FilthNode::Depth() {return depth;}
void FilthNode::Depth(int val) {depth=val;}
Coordinate FilthNode::Position() {return Coordinate(x,y);}
