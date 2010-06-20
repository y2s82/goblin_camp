#include <string>
#include <queue>
#include <set>
#ifdef DEBUG
#include <iostream>
#endif

#include "Tile.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Game.hpp"

Tile::Tile(TileType newType, int newCost) :
	vis(true),
	walkable(true),
	buildable(true),
	_moveCost(newCost),
	construction(-1),
	low(false),
	blocksWater(false),
	water(boost::shared_ptr<WaterNode>()),
	graphic('.'),
	color(TCODColor::white),
	natureObject(-1),
	filth(boost::shared_ptr<FilthNode>())
{
    type(newType);
}

TileType Tile::type() { return _type; }

void Tile::type(TileType newType) {
	_type = newType;
	if (_type == TILEGRASS) {
	    vis = true; walkable = true; buildable = true;
	    color = TCODColor(rand() % 190, 127, 0);
	    switch ((rand() % 10)) {
	        case 0:
	        case 1:
	        case 2:
	        case 3: graphic = '.'; break;
	        case 4:
	        case 5:
            case 6:
            case 7: graphic = ','; break;
            case 8: graphic = ':'; break;
            case 9: graphic = '\''; break;
	    }
    }
	else if (_type == TILEDITCH || _type == TILERIVERBED) {
	    vis = true; walkable = true; buildable = false; low = true;
	    graphic = '_';
	    color = TCODColor(125,50,0);
    }
	else { vis = false; walkable = false; buildable = false; }
}

bool Tile::BlocksLight() const { return !vis; }
void Tile::BlocksLight(bool value) { vis = !value; }

bool Tile::Walkable() const {
	if (water && water->Depth() > WALKABLE_WATER_DEPTH) return false;
	return walkable;
}
void Tile::Walkable(bool value) {
	std::queue<int> bumpQueue;
	walkable = value;
	if (value == false) {
		//This is a tough one, we need to call BumpNPC for each npc, which will modify
		//npcList. That's why we temporarily store the uids elsewhere so that we can safely
		//call them. Iterating through a list while modifying it destructively isn't safe
        for (std::set<int>::iterator npcIter = npcList.begin(); npcIter != npcList.end(); ++npcIter) {
            bumpQueue.push(*npcIter);
        }
        for (std::set<int>::iterator itemIter = itemList.begin(); itemIter != itemList.end(); ++itemIter) {
            bumpQueue.push(*itemIter);
        }
        while (!bumpQueue.empty()) { Game::Inst()->BumpEntity(bumpQueue.front()); bumpQueue.pop(); }
    }
}

bool Tile::BlocksWater() const { return blocksWater; }
void Tile::BlocksWater(bool value) { blocksWater = value; }

int Tile::moveCost() const {
    int cost = _moveCost;
	if (water) cost += water->Depth();
    if (construction >= 0) cost += 1;
    return cost;
}
void Tile::moveCost(int value) { _moveCost = value; }

void Tile::Buildable(bool value) { buildable = value; }
bool Tile::Buildable() const { return buildable; }

void Tile::MoveFrom(int uid) {
	if (npcList.find(uid) == npcList.end()) {
#ifdef DEBUG
		std::cout<<"\nNPC "<<uid<<" moved off of empty list";
		std::cout<<"\nlist.size(): "<<npcList.size();
		std::cout<<"\nNPC: "<<Game::Inst()->npcList[uid]->Position().x()<<","<<Game::Inst()->npcList[uid]->Position().y()<<'\n';
#endif
		return;
	}
	npcList.erase(npcList.find(uid));
}

bool Tile::MoveTo(int uid) {
	if (Walkable()) {
		npcList.insert(uid);
		if (npcList.find(uid) == npcList.end()) Logger::Inst()->output<<"NPC inserted but disappeared!?\n";
		return true;
	}
#ifdef DEBUG
	std::cout<<"MoveTo returned false\n";
#endif
	return false;
}

void Tile::Construction(int uid) { construction = uid; }
int Tile::Construction() const { return construction; }

boost::weak_ptr<WaterNode> Tile::GetWater() const {return boost::weak_ptr<WaterNode>(water);}
void Tile::SetWater(boost::shared_ptr<WaterNode> value) {water = value;}

bool Tile::Low() const {return low;}
void Tile::Low(bool value) {low = value;}

int Tile::Graphic() const { return graphic; }
TCODColor Tile::Color() const { return color; }

void Tile::NatureObject(int val) { natureObject = val; }
int Tile::NatureObject() const { return natureObject; }

boost::weak_ptr<FilthNode> Tile::GetFilth() const {return boost::weak_ptr<FilthNode>(filth);}
void Tile::SetFilth(boost::shared_ptr<FilthNode> value) {filth = value;}