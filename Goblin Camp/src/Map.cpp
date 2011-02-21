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

#include <boost/unordered_set.hpp>

#include "Random.hpp"
#include "Map.hpp"
#include "Game.hpp"
#include "NPC.hpp"
#include "StatusEffect.hpp"
#include "Construction.hpp"
#include "Door.hpp"
#include "MapMarker.hpp"
#include "Faction.hpp"
#include "Weather.hpp"
#include "GCamp.hpp"

Map::Map() :
overlayFlags(0), markerids(0) {
	tileMap.resize(boost::extents[500][500]);
	for (int i = 0; i < (signed int)tileMap.size(); ++i) {
		for (int e = 0; e < (signed int)tileMap[0].size(); ++e) {
			tileMap[i][e].ResetType(TILEGRASS);
		}
	}
	width = tileMap.size();
	height = tileMap[0].size();
	heightMap = new TCODHeightMap(500,500);
	waterlevel = -0.8f;
	weather = boost::shared_ptr<Weather>(new Weather(this));
};

Map::~Map() {
	delete heightMap;
}

Map* Map::instance = 0;

Map* Map::Inst() {
	if (!instance) instance = new Map();
	return instance;
}

float Map::getWalkCost(int x0, int y0, int x1, int y1, void* ptr) const {
	if (static_cast<NPC*>(ptr)->HasEffect(FLYING)) return 1.0f;
	return (float)tileMap[x1][y1].GetMoveCost(ptr);
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
TileType Map::GetType(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetType(); 
	return TILENONE;
}
void Map::ResetType(int x, int y, TileType ntype) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].ResetType(ntype); 
}

void Map::ChangeType(int x, int y, TileType ntype) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].ChangeType(ntype); 
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

boost::weak_ptr<WaterNode> Map::GetWater(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetWater();
	return boost::weak_ptr<WaterNode>();
}
void Map::SetWater(int x, int y, boost::shared_ptr<WaterNode> value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetWater(value); 
}

bool Map::IsLow(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].IsLow();
	return false;
}
void Map::SetLow(int x, int y, bool value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetLow(value); 
}

bool Map::BlocksWater(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].BlocksWater(); 
	return true;
}
void Map::SetBlocksWater(int x, int y, bool value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetBlocksWater(value); 
}

std::set<int>* Map::NPCList(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return &tileMap[x][y].npcList; 
	return &tileMap[0][0].npcList;
}
std::set<int>* Map::ItemList(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return &tileMap[x][y].itemList;
	return &tileMap[0][0].itemList;
}

int Map::GetGraphic(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetGraphic(); 
	return '?';
}
TCODColor Map::GetForeColor(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetForeColor(); 
	return TCODColor::pink;
}

void Map::ForeColor(int x, int y, TCODColor color) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		tileMap[x][y].originalForeColor = color;
		tileMap[x][y].foreColor = color;
	}
}

TCODColor Map::GetBackColor(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetBackColor(); 
	return TCODColor::yellow;
}

void Map::SetNatureObject(int x, int y, int val) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetNatureObject(val); 
}
int Map::GetNatureObject(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetNatureObject(); 
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

boost::weak_ptr<FireNode> Map::GetFire(int x, int y) { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetFire(); 
	return boost::weak_ptr<FireNode>();
}
void Map::SetFire(int x, int y, boost::shared_ptr<FireNode> value) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetFire(value); 
}

bool Map::BlocksLight(int x, int y) const { 
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].BlocksLight(); 
	return true;
}
void Map::SetBlocksLight(int x, int y, bool val) { 
	if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].SetBlocksLight(val); 
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
	tileMap[x][y].ResetType(TILEGRASS);
	tileMap[x][y].SetWalkable(true);
	tileMap[x][y].SetBuildable(true);
	tileMap[x][y].SetConstruction(-1);
	tileMap[x][y].SetWater(boost::shared_ptr<WaterNode>());
	tileMap[x][y].SetLow(false);
	tileMap[x][y].SetBlocksWater(false);
	tileMap[x][y].SetBlocksLight(false);
	tileMap[x][y].SetNatureObject(-1);
	tileMap[x][y].itemList.clear();
	tileMap[x][y].npcList.clear();
	tileMap[x][y].SetFilth(boost::shared_ptr<FilthNode>());
	tileMap[x][y].SetBlood(boost::shared_ptr<BloodNode>());
	tileMap[x][y].marked = false;
	tileMap[x][y].walkedOver = 0;
	tileMap[x][y].corruption = 0;
	tileMap[x][y].territory = false;
	tileMap[x][y].burnt = 0;
	heightMap->setValue(x,y,0.5f);
	waterlevel = -0.8f;
	overlayFlags = 0;
	mapMarkers.clear();
	markerids = 0;
	weather.reset(new Weather(this));
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
	if (construction && !bridge) modifier += construction->GetMoveSpeedModifier();

	//Other critters slow down movement
	if (tileMap[x][y].npcList.size() > 0) modifier += 2 + Random::Generate(tileMap[x][y].npcList.size() - 1);

	return modifier;
}

float Map::GetWaterlevel() { return waterlevel; }

bool Map::GroundMarked(int x, int y) { return tileMap[x][y].marked; }

void Map::WalkOver(int x, int y) { if (x >= 0 && x < width && y >= 0 && y < height) tileMap[x][y].WalkOver(); }
void Map::Corrupt(int x, int y, int magnitude) {
	int loops = 0;
	while (magnitude > 0 && loops < 2000) {
		++loops;

		if (x < 0) x = 0;
		if (x >= Width()) x = Width()-1;
		if (y < 0) y = 0;
		if (y >= Height()) y = Height()-1;

		if (tileMap[x][y].corruption < 300) {
			int difference = 300 - tileMap[x][y].corruption;
			if (magnitude - difference <= 0) {
				tileMap[x][y].Corrupt(magnitude);
				magnitude = 0;
			} else {
				tileMap[x][y].Corrupt(difference);
				magnitude -= difference;
			}

			if (tileMap[x][y].corruption >= 100) {
				if (tileMap[x][y].natureObject >= 0 && 
					!NatureObject::Presets[Game::Inst()->natureList[tileMap[x][y].natureObject]->Type()].evil &&
					!boost::iequals(Game::Inst()->natureList[tileMap[x][y].natureObject]->Name(),"Withering tree")) {
						bool createTree = Game::Inst()->natureList[tileMap[x][y].natureObject]->Tree();
						Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[tileMap[x][y].natureObject]);
						if (createTree && Random::Generate(3) < 3) Game::Inst()->CreateNatureObject(Coordinate(x,y), "Withering tree");
				}
			}
		}

		x += Random::Generate(-1, 1);
		y += Random::Generate(-1, 1);
	}
}

void Map::Naturify(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		if (tileMap[x][y].walkedOver > 0) --tileMap[x][y].walkedOver;
		if (tileMap[x][y].burnt > 0) tileMap[x][y].Burn(-1);
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
				if (IsWalkable(x, y, npc) && tileMap[x][y].npcList.size() == 0 && !IsUnbridgedWater(x,y) &&
					!IsDangerous(x, y, static_cast<NPC*>(npc)->GetFaction())) {
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
			if (water->Depth() > 0 && (!construction || !construction->Built() || !construction->HasTag(BRIDGE))) return true;
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
	if (x >= 0 && x < width && y >= 0 && y < height) return tileMap[x][y].GetForeColor();
	return TCODColor::white;
}

void Map::Burn(int x, int y, int magnitude) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		tileMap[x][y].Burn(magnitude);
	}
}

int Map::Burnt(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		return tileMap[x][y].burnt;
	}
	return 0;
}

Map::MarkerIterator Map::MarkerBegin()
{
	return mapMarkers.begin();
}

Map::MarkerIterator Map::MarkerEnd()
{
	return mapMarkers.end();
}

Direction Map::GetWindDirection() { return weather->GetWindDirection(); }
void Map::RandomizeWind() { weather->RandomizeWind(); }
void Map::ShiftWind() { weather->ShiftWind(); }
std::string Map::GetWindAbbreviation() { return weather->GetWindAbbreviation(); }

void Map::CalculateFlow(int px[4], int py[4]) {

	Direction startDirectionA, startDirectionB;
	Direction midDirectionA, midDirectionB;
	Direction endDirectionA, endDirectionB;

	if (px[0] < px[1]) startDirectionA = EAST;
	else if (px[0] > px[1]) startDirectionA = WEST;
	else startDirectionA = Random::GenerateBool() ? EAST : WEST;
	if (py[0] < py[1]) startDirectionB = SOUTH;
	else if (py[0] > py[1]) startDirectionB = NORTH;
	else startDirectionB = Random::GenerateBool() ? SOUTH : NORTH;

	if (px[1] < px[2]) midDirectionA = EAST;
	else if (px[1] > px[2]) midDirectionA = WEST;
	else midDirectionA = Random::GenerateBool() ? EAST : WEST;
	if (py[1] < py[2]) midDirectionB = SOUTH;
	else if (py[1] > py[2]) midDirectionB = NORTH;
	else midDirectionB = Random::GenerateBool() ? SOUTH : NORTH;

	if (px[2] < px[3]) endDirectionA = EAST;
	else if (px[2] > px[3]) endDirectionA = WEST;
	else endDirectionA = Random::GenerateBool() ? EAST : WEST;
	if (py[2] < py[3]) endDirectionB = SOUTH;
	else if (py[2] > py[3]) endDirectionB = NORTH;
	else endDirectionB = Random::GenerateBool() ? SOUTH : NORTH;

	if (Random::GenerateBool()) { //Reverse?
		if (startDirectionA == EAST) startDirectionA = WEST;
		else startDirectionA = EAST;
		if (startDirectionB == SOUTH) startDirectionB = NORTH;
		else startDirectionB = SOUTH;
		if (midDirectionA == EAST) midDirectionA = WEST;
		else midDirectionA = EAST;
		if (midDirectionB == SOUTH) midDirectionB = NORTH;
		else midDirectionB = SOUTH;
		if (endDirectionA == EAST) endDirectionA = WEST;
		else endDirectionA = EAST;
		if (endDirectionB == SOUTH) endDirectionB = NORTH;
		else endDirectionB = SOUTH;
	}

	Coordinate beginning(px[0], py[0]);

	boost::unordered_set<Coordinate> touched;
	std::priority_queue<std::pair<int, Coordinate> > unfinished;

	unfinished.push(std::pair<int, Coordinate>(0, beginning));
	touched.insert(beginning);

	Direction flowDirectionA, flowDirectionB;
	int stage = 0;
	bool favorA = false;
	bool favorB = false;
	int distance1 = Distance(Coordinate(px[0], py[0]), Coordinate(px[1], py[1]));
	int distance2 = Distance(Coordinate(px[1], py[1]), Coordinate(px[2], py[2]));

	if (std::abs(px[0] - px[1]) - std::abs(py[0] - py[1]) > 15)
		favorA = true;
	else if (std::abs(px[0] - px[1]) - std::abs(py[0] - py[1]) < 15)
		favorB = true;

	while (!unfinished.empty()) {
		Coordinate current = unfinished.top().second;
		unfinished.pop();

		switch (stage) {
		case 0:
			flowDirectionA = startDirectionA;
			flowDirectionB = startDirectionB;
			break;

		case 1:
			flowDirectionA = midDirectionA;
			flowDirectionB = midDirectionB;
			break;

		case 2:
			flowDirectionA = endDirectionA;
			flowDirectionB = endDirectionB;
			break;
		}

		int resultA = Random::Generate(favorA ? 3 : 1);
		int resultB = Random::Generate(favorB ? 3 : 1);
		if (resultA == resultB) Random::GenerateBool() ? resultA += 1 : resultB += 1;
		if (resultA > resultB)
			tileMap[current.X()][current.Y()].flow = flowDirectionA;
		else
			tileMap[current.X()][current.Y()].flow = flowDirectionB;

		for (int y = current.Y()-1; y <= current.Y()+1; ++y) {
			for (int x = current.X()-1; x <= current.X()+1; ++x) {
				if (x > 0 && x < Width() &&
					y > 0 && y < Height()) {
						if (touched.find(Coordinate(x,y)) == touched.end() && tileMap[x][y].water) {
							int distance = Distance(beginning, Coordinate(x,y));
							touched.insert(Coordinate(x,y));
							unfinished.push(std::pair<int, Coordinate>(INT_MAX - distance, Coordinate(x,y)));
							if (stage == 0 && distance > distance1) {
								stage = 1;
								favorA = false;
								favorB = false;
								if (std::abs(px[1] - px[2]) - std::abs(py[1] - py[2]) > 15)
									favorA = true;
								else if (std::abs(px[1] - px[2]) - std::abs(py[1] - py[2]) < 15)
									favorB = true;
							}
							if (stage == 1 && Distance(Coordinate(x,y), Coordinate(px[1], py[1])) > distance2) {
								stage = 2;
								favorA = false;
								favorB = false;
								if (std::abs(px[2] - px[3]) - std::abs(py[2] - py[3]) > 15)
									favorA = true;
								else if (std::abs(px[2] - px[3]) - std::abs(py[2] - py[3]) < 15)
									favorB = true;
							}
						}
				}
			}
		}
	}

	//Calculate flow for all ground tiles
	for (int y = 0; y < Height(); ++y) {
		for (int x = 0; x < Width(); ++x) {
			if (tileMap[x][y].flow == NODIRECTION) {
				Coordinate lowest(x,y);
				for (int iy = y-1; iy <= y+1; ++iy) {
					for (int ix = x-1; ix <= x+1; ++ix) {
						if (iy >= 0 && iy < Height() && ix >= 0 && ix < Width()) {
							if (heightMap->getValue(ix, iy) < heightMap->getValue(lowest.X(), lowest.Y())) {
								lowest = Coordinate(ix, iy);
							}
						}
					}
				}

				if (lowest.X() < x) {
					if (lowest.Y() < y)
						tileMap[x][y].flow = NORTHWEST;
					else if (lowest.Y() == y)
						tileMap[x][y].flow = WEST;
					else 
						tileMap[x][y].flow = SOUTHWEST;
				} else if (lowest.X() == x) {
					if (lowest.Y() < y)
						tileMap[x][y].flow = NORTH;
					else if (lowest.Y() > y)
						tileMap[x][y].flow = SOUTH;
				} else {
					if (lowest.Y() < y)
						tileMap[x][y].flow = NORTHEAST;
					else if (lowest.Y() == y)
						tileMap[x][y].flow = EAST;
					else
						tileMap[x][y].flow = SOUTHEAST;
				}

				if (tileMap[x][y].flow == NODIRECTION) { //No slope here, so approximate towards river
					boost::weak_ptr<WaterNode> randomWater;
					if (Game::Inst()->waterList.size() > 0) randomWater = *boost::next(Game::Inst()->waterList.begin(), Random::ChooseIndex(Game::Inst()->waterList));
					Coordinate coord(-1, -1);
					if (randomWater.lock()) coord = randomWater.lock()->Position();
					
					if (coord != Coordinate(-1, -1)) {
						if (coord.X() < x) {
							if (coord.Y() < y)
								tileMap[x][y].flow = NORTHWEST;
							else if (coord.Y() == y)
								tileMap[x][y].flow = WEST;
							else 
								tileMap[x][y].flow = SOUTHWEST;
						} else if (coord.X() == x) {
							if (coord.Y() < y)
								tileMap[x][y].flow = NORTH;
							else if (coord.Y() > y)
								tileMap[x][y].flow = SOUTH;
						} else {
							if (coord.Y() < y)
								tileMap[x][y].flow = NORTHEAST;
							else if (coord.Y() == y)
								tileMap[x][y].flow = EAST;
							else
								tileMap[x][y].flow = SOUTHEAST;
						}
					}
				}
			}
		}
	}
}

Direction Map::GetFlow(int x, int y) {
	if (x >= 0 && x < width && y >= 0 && y < height)
		return tileMap[x][y].flow;
	return NODIRECTION;
}

bool Map::IsDangerous(int x, int y, int faction) const {
	if (x >= 0 && x < width && y >= 0 && y < height) {
		if (tileMap[x][y].fire) return true;
		return Faction::factions[faction]->IsTrapVisible(Coordinate(x,y));
	}
	return false;
}

int Map::GetTerrainMoveCost(int x, int y) const {
	if (x >= 0 && x < width && y >= 0 && y < height)
		return tileMap[x][y].GetTerrainMoveCost();
	return 0;
}

void Map::Update() {
	if (Random::Generate(UPDATES_PER_SECOND * 1) == 0) Naturify(Random::Generate(Width() - 1), Random::Generate(Height() - 1));
	UpdateMarkers();
	weather->Update();
}