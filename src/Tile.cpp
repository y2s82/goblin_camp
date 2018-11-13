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
#include "stdafx.hpp"

#include <string>
#include <queue>
#include <set>
#ifdef DEBUG
#include <iostream>
#endif

#include <boost/algorithm/string.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/set.hpp>

#include "Random.hpp"
#include "Tile.hpp"
#include "Announce.hpp"
#include "Logger.hpp"
#include "Game.hpp"
#include "Construction.hpp"
#include "Faction.hpp"
#include "Trap.hpp"

Tile::Tile(TileType newType, int newCost) :
	vis(true),
	walkable(true),
	buildable(true),
	moveCost(newCost),
	construction(-1),
	low(false),
	blocksWater(false),
	water(boost::shared_ptr<WaterNode>()),
	graphic('.'),
	foreColor(TCODColor::white),
	originalForeColor(TCODColor::white),
	backColor(TCODColor::black),
	natureObject(-1),
	npcList(std::set<int>()),
	itemList(std::set<int>()),
	filth(boost::shared_ptr<FilthNode>()),
	blood(boost::shared_ptr<BloodNode>()),
	fire(boost::shared_ptr<FireNode>()),
	marked(false),
	walkedOver(0),
	corruption(0),
	territory(false),
	burnt(0),
	flow(NODIRECTION)
{
	ResetType(newType);
}

TileType Tile::GetType() { return type; }

void Tile::ResetType(TileType newType, float height) {
	type = newType;
	if (type == TILEGRASS) {
		vis = true; walkable = true; buildable = true; low = false;
		originalForeColor = TCODColor(Random::Generate(49), 127, 0);
		if (Random::Generate(9) < 9) {
			if (height < -0.01f) {
				originalForeColor = TCODColor(Random::Generate(100,192),127,0);
			} else if (height < 0.0f) {
				originalForeColor = TCODColor(Random::Generate(20,170),127,0);
			} else if (height > 4.0f) {
				originalForeColor = TCODColor(90, Random::Generate(120,150), 90);
			}
		}
		backColor = TCODColor(0, 0, 0);
		switch (Random::Generate(9)) {
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
	} else if (type == TILEDITCH || type == TILERIVERBED) {
		vis = true; walkable = true; buildable = true; low = true;
		graphic = '_';
		originalForeColor = TCODColor(125,50,0);
		moveCost = Random::Generate(3, 5);
		flow = NODIRECTION; //Reset flow
	} else if (type == TILEBOG) {
		vis = true; walkable = true; buildable = true; low = false;
		switch (Random::Generate(9)) {
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
		originalForeColor = TCODColor(Random::Generate(184), 127, 70);
		backColor = TCODColor(60,30,20);
		moveCost = Random::Generate(6, 10);
	} else if (type == TILEROCK) {
		vis = true; walkable = true; buildable = true; low = false;
		graphic = (Random::GenerateBool() ? ',' : '.');
		originalForeColor = TCODColor(Random::Generate(182, 182 + 19), Random::Generate(182, 182 + 19), Random::Generate(182, 182 + 19));
		backColor = TCODColor(0, 0, 0);
	} else if (type == TILEMUD) {
		vis = true; walkable = true; buildable = true; low = false;
		graphic = Random::GenerateBool() ? '#' : '~';
		originalForeColor = TCODColor(Random::Generate(120, 130), Random::Generate(80, 90), 0);
		backColor = TCODColor(0, 0, 0);
		moveCost = 5;
	} else if (type == TILESNOW) {
		vis = true; walkable = true; buildable = true; low = false;
		int colorNum = Random::Generate(195,250);
		originalForeColor = TCODColor(colorNum + Random::Generate(-5, 5), colorNum + Random::Generate(-5, 5), 
			colorNum + Random::Generate(-5, 5));
		backColor = TCODColor(0, 0, 0);
		switch (Random::Generate(9)) {
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
	} else { vis = false; walkable = false; buildable = false; }
	foreColor = originalForeColor;
}

void Tile::ChangeType(TileType newType,float height) {
	bool oldBuildable = buildable;
	bool oldVis = vis; 
	bool oldWalkable = walkable; 
	int oldGraphic = graphic;
	bool keepGraphic = (type == TILEGRASS || type == TILESNOW) && (newType == TILEGRASS || newType == TILESNOW);
	ResetType(newType,height);
	buildable = oldBuildable;
	vis = oldVis;
	walkable = oldWalkable;
	if (keepGraphic) {
		graphic = oldGraphic;
		Corrupt(0); //Recalculates color
	}
}

bool Tile::BlocksLight() const { return !vis; }
void Tile::SetBlocksLight(bool value) { vis = !value; }

bool Tile::IsWalkable() const {
	return walkable;
}
void Tile::SetWalkable(bool value) {
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
void Tile::SetBlocksWater(bool value) { blocksWater = value; }

int Tile::GetTerrainMoveCost() const {
	int cost = moveCost;
	if (construction >= 0) cost += 2;
	return cost;
}

void Tile::SetMoveCost(int value) { moveCost = value; }

void Tile::SetBuildable(bool value) { buildable = value; }
bool Tile::IsBuildable() const { return buildable; }

void Tile::MoveFrom(int uid) {
	if (npcList.find(uid) == npcList.end()) {
#ifdef DEBUG
		std::cout<<"\nNPC "<<uid<<" moved off of empty list";
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

bool Tile::IsLow() const {return low;}
void Tile::SetLow(bool value) {low = value;}

int Tile::GetGraphic() const { return graphic; }
TCODColor Tile::GetForeColor() const { 
	return foreColor;
}
TCODColor Tile::GetBackColor() const {
	if (!blood && !marked) return backColor;
	TCODColor result = backColor;
	if (blood)
		result.r = std::min(255, backColor.r + blood->Depth());
	if (marked)
		result = result + TCODColor::darkGrey;
	return result; 
}

void Tile::SetNatureObject(int val) { natureObject = val; }
int Tile::GetNatureObject() const { return natureObject; }

boost::weak_ptr<FilthNode> Tile::GetFilth() const {return boost::weak_ptr<FilthNode>(filth);}
void Tile::SetFilth(boost::shared_ptr<FilthNode> value) {filth = value;}

boost::weak_ptr<BloodNode> Tile::GetBlood() const {return boost::weak_ptr<BloodNode>(blood);}
void Tile::SetBlood(boost::shared_ptr<BloodNode> value) {blood = value;}

boost::weak_ptr<FireNode> Tile::GetFire() const {return boost::weak_ptr<FireNode>(fire);}
void Tile::SetFire(boost::shared_ptr<FireNode> value) { fire = value; }

void Tile::Mark() { marked = true; }
void Tile::Unmark() { marked = false; }

void Tile::WalkOver() {
	//Ground under a construction wont turn to mud
	if (walkedOver < 120 || construction < 0) ++walkedOver;
	if (type == TILEGRASS) {
		foreColor = originalForeColor + TCODColor(std::min(255, walkedOver), 0, 0) - TCODColor(0, std::min(255,corruption), 0);
		if (burnt > 0) Burn(0); //Just to re-do the color
		if (walkedOver > 100 && graphic != '.' && graphic != ',') graphic = Random::GenerateBool() ? '.' : ',';
		if (walkedOver > 300 && Random::Generate(99) == 0) ChangeType(TILEMUD);
	}
}

void Tile::Corrupt(int magnitude) {
	corruption += magnitude;
	if (corruption < 0) corruption = 0;
	if (type == TILEGRASS) {
		foreColor = originalForeColor + TCODColor(std::min(255, walkedOver), 0, 0) - TCODColor(0, std::min(255,corruption), 0);;
		if (burnt > 0) Burn(0); //Just to re-do the color
	}
}

TileType Tile::StringToTileType(std::string string) {
	if (boost::iequals(string, "grass")) {
		return TILEGRASS;
	} else if (boost::iequals(string, "river")) {
		return TILERIVERBED;
	} else if (boost::iequals(string, "ditch")) {
		return TILEDITCH;
	} else if (boost::iequals(string, "rock")) {
		return TILEROCK;
	} else if (boost::iequals(string, "mud")) {
		return TILEMUD;
	} else if (boost::iequals(string, "bog")) {
		return TILEBOG;
	}
	return TILENONE;
}

void Tile::Burn(int magnitude) {
	if (type == TILEGRASS) {
		burnt = std::min(10, burnt + magnitude);
		burnt = std::max(0, burnt);
		if (burnt == 0) {
			Corrupt(0); /*Corruption changes the color, and by corrupting by 0 we just return to what color the tile
						would be without any burning*/
			return;
		}

		if (type == TILEGRASS) {
			if (burnt < 5) {
				foreColor.r = 130 + ((5 - burnt) * 10);
				foreColor.g = 80 + ((5 - burnt) * 5);
				foreColor.b = 0;
			} else {
				foreColor.r = 50 + ((10 - burnt) * 12);
				foreColor.g = 50 + ((10 - burnt) * 6);
				foreColor.b = (burnt - 5) * 10;
			}
		}
	}
}

void Tile::save(OutputArchive& ar, const unsigned int version) const {
	ar & type;
	ar & vis;
	ar & walkable;
	ar & buildable;
	ar & moveCost;
	ar & construction;
	ar & low;
	ar & blocksWater;
	ar & water;
	ar & graphic;
	ar & foreColor.r;
	ar & foreColor.g;
	ar & foreColor.b;
	ar & originalForeColor.r;
	ar & originalForeColor.g;
	ar & originalForeColor.b;
	ar & backColor.r;
	ar & backColor.g;
	ar & backColor.b;
	ar & natureObject;
	ar & npcList;
	ar & itemList;
	ar & filth;
	ar & blood;
	ar & marked;
	ar & walkedOver;
	ar & corruption;
	ar & territory;
	ar & burnt;
	ar & fire;
	ar & flow;
}

void Tile::load(InputArchive& ar, const unsigned int version) {
	ar & type;
	ar & vis;
	ar & walkable;
	ar & buildable;
	ar & moveCost;
	ar & construction;
	ar & low;
	ar & blocksWater;
	ar & water;
	ar & graphic;
	ar & foreColor.r;
	ar & foreColor.g;
	ar & foreColor.b;
	ar & originalForeColor.r;
	ar & originalForeColor.g;
	ar & originalForeColor.b;
	ar & backColor.r;
	ar & backColor.g;
	ar & backColor.b;
	ar & natureObject;
	ar & npcList;
	ar & itemList;
	ar & filth;
	ar & blood;
	ar & marked;
	ar & walkedOver;
	ar & corruption;
	ar & territory;
	ar & burnt;
	ar & fire;
	ar & flow;
}

CacheTile::CacheTile() : walkable(true), moveCost(1), construction(false),
	door(false), trap(false), bridge(false), moveSpeedModifier(0),
	waterDepth(0), npcCount(0), fire(false), x(0), y(0) {}

CacheTile& CacheTile::operator=(const Tile& tile) {
	walkable = tile.walkable;
	moveCost = tile.moveCost;
	boost::shared_ptr<Construction> construct = Game::Inst()->GetConstruction(tile.construction).lock();
	if (construct) {
		construction = true;
		door = construct->HasTag(DOOR);
		trap = construct->HasTag(TRAP);
		bridge = construct->HasTag(BRIDGE);
		moveSpeedModifier = construct->GetMoveSpeedModifier();
	} else {
		construction = false;
		door = false;
		trap = false;
		bridge = false;
		moveSpeedModifier = 0;
	}

	if (tile.water) waterDepth = tile.water->Depth();
	else waterDepth = 0;

	npcCount = tile.npcList.size();
	fire = bool(tile.fire);

	return *this;
}

int CacheTile::GetMoveCost(void* ptr) const {
	int cost = GetMoveCost();

	if (cost < 100) { //If we're over 100 then it's clear enough that walking here is not a good choice

		NPC* npc = static_cast<NPC*>(ptr);

		if (npc) {
			if (door && !npc->HasHands()) {
				cost += 50;
			}
			if (trap) { 
				cost = Faction::factions[npc->GetFaction()]->IsTrapVisible(Coordinate(x,y)) ? 
					100 : 1;
			}

			//cost == 0 normally means unwalkable, but tunnellers can, surprise surprise, tunnel through
			if (cost == 0 && construction && npc->IsTunneler()) cost = 50;
		}
	}
	return cost;
}

int CacheTile::GetMoveCost() const {
	if (!walkable) return 0;
	int cost = moveCost;

	if (fire) cost += 500; //Walking through fire... not such a good idea.

	//If a construction exists here take it into consideration
	if (bridge) {
		cost -= (moveCost-1); //Disregard terrain in case of a bridge
	}
	cost += moveSpeedModifier;
	
	if (!bridge) { //If no built bridge here take water depth into account
		cost += std::min(20, waterDepth);
	}

	return cost;
}
