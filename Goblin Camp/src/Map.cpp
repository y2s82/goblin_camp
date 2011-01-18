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

#include "Random.hpp"
#include "Map.hpp"
#include "Game.hpp"
#include "NPC.hpp"
#include "StatusEffect.hpp"
#include "Construction.hpp"
#include "Door.hpp"
#include "MapMarker.hpp"

Map::Map() :
overlayFlags(0), markerids(0) {
	tileMap.resize(boost::extents[500][500]);
	for (int i = 0; i < (signed int)tileMap.size(); ++i) {
		for (int e = 0; e < (signed int)tileMap[0].size(); ++e) {
			tileMap[i][e].SetType(TILEGRASS);
		}
	}
	width = tileMap.size();
	height = tileMap[0].size();
	heightMap = new TCODHeightMap(500,500);
	waterlevel = -0.8f;
};

Map* Map::instance = 0;

Map* Map::Inst() {
	if (!instance) instance = new Map();
	return instance;
}

float Map::getWalkCost(int x0, int y0, int x1, int y1, void* ptr) const {
	if (static_cast<NPC*>(ptr)->HasEffect(FLYING)) return 1.0f;
	return (float)tileMap[x0][y0].GetMoveCost(ptr);
}

//Simple version that doesn't take npc information into account
bool Map::IsWalkable(int x, int y) const {
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].IsWalkable();
	return false;
}

bool Map::IsWalkable(int x, int y, void* ptr) const {
	if (static_cast<NPC*>(ptr)->HasEffect(FLYING)) return true;
	if (!static_cast<NPC*>(ptr)->HasHands()) {
		if (GetConstruction(x,y) >= 0) {
			if (boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(GetConstruction(x,y)).lock()) {
				if (cons->HasTag(DOOR) && !boost::static_pointer_cast<Door>(cons)->Open()) {
					return false;
				}
			}
		}
	}
	return IsWalkable(x,y);
}

void Map::SetWalkable(int x,int y, bool value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetWalkable(value); 
}

int Map::Width() { return width; }
int Map::Height() { return height; }
bool Map::IsBuildable(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].IsBuildable(); 
	return false;
}
void Map::SetBuildable(int x, int y, bool value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetBuildable(value); 
}
TileType Map::Type(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetType(); 
	return TILENONE;
}
void Map::Type(int x, int y, TileType ntype) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetType(ntype); 
}
void Map::MoveTo(int x, int y, int uid) {
	if (x >= 0 && x < Width() && y >= 0 && y < Height()) {
		tileMap[x][y].MoveTo(uid);
	} 
}
void Map::MoveFrom(int x, int y, int uid) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].MoveFrom(uid); 
}

void Map::SetConstruction(int x, int y, int uid) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetConstruction(uid); 
}
int Map::GetConstruction(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetConstruction(); 
	return -1;
}

//TODO: Optimize. This causes the biggest performance hit by far right now 
void Map::Draw(Coordinate upleft, TCODConsole *console) {
	int screenDeltaX = upleft.X();
	int screenDeltaY = upleft.Y();
	for (int y = upleft.Y(); y < upleft.Y() + console->getHeight(); ++y) {
		for (int x = upleft.X(); x < upleft.X() + console->getWidth(); ++x) {
			if (x >= 0 && x < width && y >= 0 && y < height) {

				console->putCharEx(x-screenDeltaX,y-(screenDeltaY), Graphic(x,y), ForeColor(x,y), BackColor(x,y));

				boost::weak_ptr<WaterNode> water = GetWater(x,y);
				if (water.lock()) {
					water.lock()->Draw(upleft, console);
				}
				boost::weak_ptr<FilthNode> filth = GetFilth(x,y);
				if (filth.lock()) {
					filth.lock()->Draw(upleft, console);
				}

				if (overlayFlags & TERRITORY_OVERLAY) {
					console->setCharBackground(x-screenDeltaX,y-screenDeltaY, tileMap[x][y].territory ? TCODColor::darkGreen : TCODColor::darkRed);
				}
			}
			else {
				console->putCharEx(x-screenDeltaX,y-screenDeltaY, TCOD_CHAR_BLOCK3, TCODColor::black, TCODColor::white);
			}
		}
	}

	for (std::list<std::pair<unsigned int, MapMarker> >::iterator markeri = mapMarkers.begin(); markeri != mapMarkers.end(); ++markeri) {
		markeri->second.Draw(upleft, console);
	}
}

boost::weak_ptr<WaterNode> Map::GetWater(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetWater();
	return boost::weak_ptr<WaterNode>();
}
void Map::SetWater(int x, int y, boost::shared_ptr<WaterNode> value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetWater(value); 
}

bool Map::Low(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].Low();
	return false;
}
void Map::Low(int x, int y, bool value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].Low(value); 
}

bool Map::BlocksWater(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].BlocksWater(); 
	return true;
}
void Map::BlocksWater(int x, int y, bool value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].BlocksWater(value); 
}

std::set<int>* Map::NPCList(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return &tileMap[x][y].npcList; 
	return &tileMap[0][0].npcList;
}
std::set<int>* Map::ItemList(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return &tileMap[x][y].itemList;
	return &tileMap[0][0].itemList;
}

int Map::Graphic(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].Graphic(); 
	return '?';
}
TCODColor Map::ForeColor(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].ForeColor(); 
	return TCODColor::pink;
}

void Map::ForeColor(int x, int y, TCODColor color) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		tileMap[x][y].originalForeColor = color;
		tileMap[x][y].foreColor = color;
	}
}

TCODColor Map::BackColor(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].BackColor(); 
	return TCODColor::yellow;
}

void Map::NatureObject(int x, int y, int val) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].NatureObject(val); 
}
int Map::NatureObject(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].NatureObject(); 
	return -1;
}

boost::weak_ptr<FilthNode> Map::GetFilth(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetFilth(); 
	return boost::weak_ptr<FilthNode>();
}
void Map::SetFilth(int x, int y, boost::shared_ptr<FilthNode> value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetFilth(value); 
}

boost::weak_ptr<BloodNode> Map::GetBlood(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetBlood(); 
	return boost::weak_ptr<BloodNode>();
}
void Map::SetBlood(int x, int y, boost::shared_ptr<BloodNode> value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetBlood(value); 
}

bool Map::BlocksLight(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].BlocksLight(); 
	return true;
}
void Map::BlocksLight(int x, int y, bool val) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].BlocksLight(val); 
}

bool Map::LineOfSight(Coordinate a, Coordinate b) {
	return LineOfSight(a.X(), a.Y(), b.X(), b.Y());
}

bool Map::LineOfSight(int ax, int ay, int bx, int by) {
	TCODLine::init(ax, ay, bx, by);
	int x = ax;
	int y = ay;
	do {
		if (BlocksLight(x,y)) return false;
	} while(!TCODLine::step(&x, &y) && !(x == bx && y == by));
	return true;
}

void Map::Reset(int x, int y) {
	tileMap[x][y].SetType(TILEGRASS);
	tileMap[x][y].SetWalkable(true);
	tileMap[x][y].SetBuildable(true);
	tileMap[x][y].SetConstruction(-1);
	tileMap[x][y].SetWater(boost::shared_ptr<WaterNode>());
	tileMap[x][y].Low(false);
	tileMap[x][y].BlocksWater(false);
	tileMap[x][y].BlocksLight(false);
	tileMap[x][y].NatureObject(-1);
	tileMap[x][y].itemList.clear();
	tileMap[x][y].npcList.clear();
	tileMap[x][y].SetFilth(boost::shared_ptr<FilthNode>());
	tileMap[x][y].SetBlood(boost::shared_ptr<BloodNode>());
	tileMap[x][y].marked = false;
	tileMap[x][y].walkedOver = 0;
	tileMap[x][y].corruption = 0;
	tileMap[x][y].territory = false;
	heightMap->setValue(x,y,0.5f);
	waterlevel = -0.8f;
	overlayFlags = 0;
	mapMarkers.clear();
	markerids = 0;
}

void Map::Mark(int x, int y) { tileMap[x][y].Mark(); }
void Map::Unmark(int x, int y) { tileMap[x][y].Unmark(); }

int Map::GetMoveModifier(int x, int y) {
	int modifier = 0;

	boost::shared_ptr<Construction> construction;
	if (tileMap[x][y].construction >= 0) construction = Game::Inst()->GetConstruction(tileMap[x][y].construction).lock();
	bool bridge = false;
	if (construction) bridge = (construction->Built() && construction->HasTag(BRIDGE));

	if (tileMap[x][y].GetType() == TILEBOG && !bridge) modifier += 10;
	else if (tileMap[x][y].GetType() == TILEDITCH && !bridge) modifier += 4;
	else if (tileMap[x][y].GetType() == TILEMUD && !bridge) { //Mud adds 6 if there's no bridge
		modifier += 6;
	}
	if (boost::shared_ptr<WaterNode> water = tileMap[x][y].GetWater().lock()) { //Water adds 'depth' without a bridge
		if (!bridge) modifier += water->Depth();
	}

	//Constructions (except bridges) slow down movement
	if (construction && !bridge) modifier += 2;

	//Other critters slow down movement
	if (tileMap[x][y].npcList.size() > 0) modifier += 2 + rand() % tileMap[x][y].npcList.size();

	return modifier;
}

float Map::GetWaterlevel() { return waterlevel; }

bool Map::GroundMarked(int x, int y) { return tileMap[x][y].marked; }

void Map::WalkOver(int x, int y) { if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].WalkOver(); }
void Map::Corrupt(int x, int y, int magnitude) {
	while (magnitude > 40) {
		magnitude -= 40;
		Corrupt(x, y, 40);
	}
	if (x >= 0 && x < width && y >= 0 && y < height) {
		tileMap[x][y].Corrupt(magnitude);
		if (tileMap[x][y].corruption >= 100) {
			if (tileMap[x][y].natureObject >= 0 && 
				!NatureObject::Presets[Game::Inst()->natureList[tileMap[x][y].natureObject]->Type()].evil &&
				!boost::iequals(Game::Inst()->natureList[tileMap[x][y].natureObject]->Name(),"Withering tree")) {
					bool createTree = Game::Inst()->natureList[tileMap[x][y].natureObject]->Tree();
					Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[tileMap[x][y].natureObject]);
					if (createTree && Random::Generate(3) < 3) Game::Inst()->CreateNatureObject(Coordinate(x,y), "Withering tree");
			}
		}
		if (tileMap[x][y].corruption > 300) {
			tileMap[x][y].Corrupt(-40);
			Corrupt(x-1, y, 10);
			Corrupt(x+1, y, 10);
			Corrupt(x, y-1, 10);
			Corrupt(x, y+1, 10);
		}
	}
}

void Map::Naturify(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		if (tileMap[x][y].walkedOver > 0) --tileMap[x][y].walkedOver;
		if (tileMap[x][y].walkedOver == 0 && tileMap[x][y].natureObject < 0 && tileMap[x][y].construction < 0) {
			int natureObjects = 0;
			int beginx = std::max(0, x-2);
			int endx = std::min(width-2, x+2);
			int beginy = std::max(0, y-2);
			int endy = std::min(height-2, y+2);
			for (int ix = beginx; ix <= endx; ++ix) {
				for (int iy = beginy; iy <= endy; ++iy) {
					if (tileMap[ix][iy].natureObject >= 0) ++natureObjects;
				}
			}
			if (natureObjects < (tileMap[x][y].corruption < 100 ? 5 : 1)) { //Corrupted areas have less flora
				Game::Inst()->CreateNatureObject(Coordinate(x,y));
			}
		} 
	}
}

void Map::Corrupt(Coordinate location, int magnitude) { Corrupt(location.X(), location.Y(), magnitude); }

int Map::GetCorruption(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].corruption;
	return 0;
}

bool Map::IsTerritory(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].territory;
	return false;
}

void Map::SetTerritory(int x, int y, bool value) {
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].territory = value;
}

void Map::SetTerritoryRectangle(Coordinate a, Coordinate b, bool value) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			SetTerritory(x, y, value);
		}
	}
}

int Map::GetOverlayFlags() { return overlayFlags; }

void Map::AddOverlay(int flags) { overlayFlags |= flags; }
void Map::RemoveOverlay(int flags) { overlayFlags = overlayFlags & ~flags; }

void Map::ToggleOverlay(int flags) { overlayFlags ^= flags; }

void Map::FindEquivalentMoveTarget(int currentX, int currentY, int &moveX, int &moveY, int nextX, int nextY, void* npc) {
	//We need to find a tile that is walkable, and adjacent to all 3 given tiles but not the same as moveX or moveY

	//Find the edges
	int left = currentX < moveX ? currentX : moveX;
	left = left < nextX ? left : nextX;
	int right = currentX > moveX ? currentX : moveX;
	right = right > nextX ? right : nextX;

	int up = currentY < moveY ? currentY : moveY;
	up = up < nextY ? up : nextY;
	int down = currentY > moveY ? currentY : moveY;
	down = down > nextY ? down : nextY;

	--left;
	++right;
	--up;
	++down;

	if (left < 0) left = 0;
	if (right >= width) right = width-1;
	if (up < 0) up = 0;
	if (down >= height) down = height-1;

	Coordinate current(currentX, currentY);
	Coordinate move(moveX, moveY);
	Coordinate next(nextX, nextY);

	//Find a suitable target
	for (int x = left; x <= right; ++x) {
		for (int y = up; y <= down; ++y) {
			if (x != moveX || y != moveY) { //Only consider tiles not == moveX,moveY
				if (IsWalkable(x, y, npc) && tileMap[x][y].npcList.size() == 0 && !IsUnbridgedWater(x,y)) {
					Coordinate xy(x,y);
					if (Game::Adjacent(xy, current) && Game::Adjacent(xy, move) && Game::Adjacent(xy, next)) {
						moveX = x;
						moveY = y;
						return;
					}
				}
			}
		}
	}
}

bool Map::IsUnbridgedWater(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		if (boost::shared_ptr<WaterNode> water = tileMap[x][y].water) {
			boost::shared_ptr<Construction> construction = Game::Inst()->GetConstruction(tileMap[x][y].construction).lock();
			if (water->Depth() > 1 && (!construction || !construction->Built() || !construction->HasTag(BRIDGE))) return true;
		}
	}
	return false;
}

unsigned int Map::AddMarker(MapMarker marker) {
	mapMarkers.push_back(std::pair<unsigned int, MapMarker>(markerids, marker));
	++markerids;
	return markerids - 1;
}

void Map::RemoveMarker(int markid) {
	for (std::list<std::pair<unsigned int, MapMarker> >::iterator markeri = mapMarkers.begin(); markeri != mapMarkers.end(); ++markeri) {
		if (markeri->first == markid) {
			mapMarkers.erase(markeri);
			return;
		}
	}
}

void Map::UpdateMarkers() {
	for (std::list<std::pair<unsigned int, MapMarker> >::iterator markeri = mapMarkers.begin(); markeri != mapMarkers.end();) {
		if (!markeri->second.Update()) {
			markeri = mapMarkers.erase(markeri);
		} else ++markeri;
	}
}

TCODColor Map::GetColor(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].ForeColor();
	return TCODColor::white;
}