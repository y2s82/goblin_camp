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
#include "Construction.hpp"

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
	foreColor(TCODColor::white),
	backColor(TCODColor::black),
	natureObject(-1),
	npcList(std::set<int>()),
	itemList(std::set<int>()),
	filth(boost::shared_ptr<FilthNode>()),
	blood(boost::shared_ptr<BloodNode>()),
	marked(false)
{
	type(newType);
}

TileType Tile::type() { return _type; }

void Tile::type(TileType newType) {
	_type = newType;
	if (_type == TILEGRASS) {
		vis = true; walkable = true; buildable = true;
		foreColor = TCODColor(rand() % 190, 127, 0);
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
	} else if (_type == TILEDITCH || _type == TILERIVERBED) {
		vis = true; walkable = true; buildable = false; low = true;
		graphic = '_';
		foreColor = TCODColor(125,50,0);
	} else if (_type == TILEBOG) {
		vis = true; walkable = true; buildable = false; low = false;
		switch ((rand() % 10)) {
		case 0:
		case 1:
		case 2:
		case 3: graphic = '~'; break;
		case 4:
		case 5:
		case 6:
		case 7: graphic = ','; break;
		case 8: graphic = ':'; break;
		case 9: graphic = '\''; break;
		}
		foreColor = TCODColor(rand() % 185, 127, 70);
		backColor = TCODColor(60,30,20);
		_moveCost = rand() % 5 + 1;
	} else { vis = false; walkable = false; buildable = false; }
}

bool Tile::BlocksLight() const { return !vis; }
void Tile::BlocksLight(bool value) { vis = !value; }

bool Tile::Walkable() const {
	return walkable;
}
void Tile::Walkable(bool value) {
	std::queue<int> bumpQueue;
	walkable = value;
	if (value == false) {
		//We temporarily store the uids elsewhere so that we can safely
		//call them. Iterating through a set while modifying it destructively isn't safe
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

int Tile::MoveCost(void* ptr) const {
	if (!static_cast<NPC*>(ptr)->HasHands()) {
		if (construction >= 0) {
			if (boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(construction).lock()) {
				if (cons->HasTag(DOOR)) return MoveCost()+50;
			}
		}
	}
	return MoveCost();
}

int Tile::MoveCost() const {
	if (!Walkable()) return 0;
	int cost = _moveCost;
	if (water) cost += std::min(WALKABLE_WATER_DEPTH, water->Depth());
	if (construction >= 0) cost += 1;
	return cost;
}
void Tile::SetMoveCost(int value) { _moveCost = value; }

void Tile::Buildable(bool value) { buildable = value; }
bool Tile::Buildable() const { return buildable; }

void Tile::MoveFrom(int uid) {
	if (npcList.find(uid) == npcList.end()) {
#ifdef DEBUG
		std::cout<<"\nNPC "<<uid<<" moved off of empty list";
		std::cout<<"\nlist.size(): "<<npcList.size();
		std::cout<<"\nNPC: "<<Game::Inst()->npcList[uid]->Position().X()<<","<<Game::Inst()->npcList[uid]->Position().Y()<<'\n';
#endif
		return;
	}
	npcList.erase(npcList.find(uid));
}

void Tile::MoveTo(int uid) {
	npcList.insert(uid);		
}

void Tile::SetConstruction(int uid) { construction = uid; }
int Tile::GetConstruction() const { return construction; }

boost::weak_ptr<WaterNode> Tile::GetWater() const {return boost::weak_ptr<WaterNode>(water);}
void Tile::SetWater(boost::shared_ptr<WaterNode> value) {water = value;}

bool Tile::Low() const {return low;}
void Tile::Low(bool value) {low = value;}

int Tile::Graphic() const { return graphic; }
TCODColor Tile::ForeColor() const { 
	return foreColor;
}
TCODColor Tile::BackColor() const {
	if (!blood && !marked) return backColor;
	TCODColor result = backColor;
	if (blood)
		result.r = std::min(255, backColor.r + blood->Depth());
	if (marked)
		result = result + TCODColor::darkGrey;
	return result; 
}

void Tile::NatureObject(int val) { natureObject = val; }
int Tile::NatureObject() const { return natureObject; }

boost::weak_ptr<FilthNode> Tile::GetFilth() const {return boost::weak_ptr<FilthNode>(filth);}
void Tile::SetFilth(boost::shared_ptr<FilthNode> value) {filth = value;}

boost::weak_ptr<BloodNode> Tile::GetBlood() const {return boost::weak_ptr<BloodNode>(blood);}
void Tile::SetBlood(boost::shared_ptr<BloodNode> value) {blood = value;}

void Tile::Mark() { marked = true; }
void Tile::Unmark() { marked = false; }
