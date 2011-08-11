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
#include <boost/algorithm/string.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/utility.hpp>

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

static const int HARDCODED_WIDTH = 500;
static const int HARDCODED_HEIGHT = 500;

Map::Map() :
overlayFlags(0), markerids(0) {
	tileMap.resize(boost::extents[HARDCODED_WIDTH][HARDCODED_HEIGHT]);
	cachedTileMap.resize(boost::extents[HARDCODED_WIDTH][HARDCODED_HEIGHT]);
	heightMap = new TCODHeightMap(HARDCODED_WIDTH,HARDCODED_HEIGHT);
	extent = Coordinate(HARDCODED_WIDTH, HARDCODED_HEIGHT);
	for (int i = 0; i < HARDCODED_WIDTH; ++i) {
		for (int e = 0; e < HARDCODED_HEIGHT; ++e) {
			tileMap[i][e].ResetType(TILEGRASS);
			cachedTileMap[i][e].x = i;
			cachedTileMap[i][e].y = e;
		}
	}
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

Coordinate Map::Extent() { return extent; }
int Map::Width() { return extent.X(); }
int Map::Height() { return extent.Y(); }

bool Map::IsInside(const Coordinate& p) const
{
	return p.insideExtent(zero, extent);
}
Coordinate Map::Shrink(const Coordinate& p) const
{
	return p.shrinkExtent(zero, extent);
}

float Map::getWalkCost(const Coordinate& from, const Coordinate& to, void* ptr) const {
	if (static_cast<NPC*>(ptr)->IsFlying()) return 1.0f;
	return (float)cachedTile(to).GetMoveCost(ptr);
}
float Map::getWalkCost(int fx, int fy, int tx, int ty, void* ptr) const {
	return Map::getWalkCost(Coordinate(fx,fy),Coordinate(tx,ty),ptr);
}

//Simple version that doesn't take npc information into account
bool Map::IsWalkable(const Coordinate& p) const {
	return (Map::IsInside(p) && tile(p).IsWalkable());
}

bool Map::IsWalkable(const Coordinate& p, void* ptr) const {
	if (static_cast<NPC*>(ptr)->HasEffect(FLYING)) return true;
	if (!static_cast<NPC*>(ptr)->HasHands()) {
		int constructionId = GetConstruction(p);
		if (constructionId >= 0) {
			if (boost::shared_ptr<Construction> cons = Game::Inst()->GetConstruction(constructionId).lock()) {
				if (cons->HasTag(DOOR) && !boost::static_pointer_cast<Door>(cons)->Open()) {
					return false;
				}
			}
		}
	}
	return IsWalkable(p);
}

void Map::SetWalkable(const Coordinate &p, bool value) {
	if (Map::IsInside(p)) {
		tile(p).SetWalkable(value);
		changedTiles.insert(p);
	}
}

bool Map::IsBuildable(const Coordinate& p) const { 
	return Map::IsInside(p) && tile(p).IsBuildable();
}

void Map::SetBuildable(const Coordinate& p, bool value) { 
	if (Map::IsInside(p))
		tile(p).SetBuildable(value);
}

TileType Map::GetType(const Coordinate& p) { 
	if (Map::IsInside(p)) return tile(p).GetType(); 
	return TILENONE;
}

void Map::ResetType(const Coordinate& p, TileType ntype, float tileHeight) { 
	if (Map::IsInside(p)) {
		tile(p).ResetType(ntype, tileHeight);
		changedTiles.insert(p);
	}
}
void Map::ChangeType(const Coordinate& p, TileType ntype, float tileHeight) { 
	if (Map::IsInside(p)) {
		tile(p).ChangeType(ntype, tileHeight);
		changedTiles.insert(p);
	}
}

void Map::MoveTo(const Coordinate& p, int uid) {
	if (Map::IsInside(p)) {
		tile(p).MoveTo(uid);
	} 
}

void Map::MoveFrom(const Coordinate& p, int uid) { 
	if (Map::IsInside(p)) tile(p).MoveFrom(uid); 
}

void Map::SetConstruction(const Coordinate& p, int uid) { 
	if (Map::IsInside(p)) {
		tile(p).SetConstruction(uid);
		changedTiles.insert(p);
	}
}
int Map::GetConstruction(const Coordinate& p) const { 
	if (Map::IsInside(p)) return tile(p).GetConstruction(); 
	return -1;
}

boost::weak_ptr<WaterNode> Map::GetWater(const Coordinate& p) { 
	if (Map::IsInside(p)) return tile(p).GetWater();
	return boost::weak_ptr<WaterNode>();
}
void Map::SetWater(const Coordinate& p, boost::shared_ptr<WaterNode> value) { 
	if (Map::IsInside(p)) {
		tile(p).SetWater(value);
		changedTiles.insert(p);
	}
}

bool Map::IsLow(const Coordinate& p) const { 
	return Map::IsInside(p) && tile(p).IsLow();
}
void Map::SetLow(const Coordinate& p, bool value) { 
	if (Map::IsInside(p)) tile(p).SetLow(value); 
}

bool Map::BlocksWater(const Coordinate& p) const { 
	return !Map::IsInside(p) || tile(p).BlocksWater(); 
}
void Map::SetBlocksWater(const Coordinate& p, bool value) { 
	if (Map::IsInside(p)) {
		tile(p).SetBlocksWater(value);
	}
}

std::set<int>* Map::NPCList(const Coordinate& p) { 
	if (Map::IsInside(p)) return &tile(p).npcList; 
	return &tileMap[0][0].npcList;
}
std::set<int>* Map::ItemList(const Coordinate& p) { 
	if (Map::IsInside(p)) return &tile(p).itemList;
	return &tileMap[0][0].itemList;
}

int Map::GetGraphic(const Coordinate& p) const { 
	if (Map::IsInside(p)) return tile(p).GetGraphic(); 
	return '?';
}
TCODColor Map::GetForeColor(const Coordinate& p) const { 
	if (Map::IsInside(p)) return tile(p).GetForeColor(); 
	return TCODColor::pink;
}

void Map::ForeColor(const Coordinate& p, TCODColor color) {
	if (Map::IsInside(p)) {
		tile(p).originalForeColor = color;
		tile(p).foreColor = color;
	}
}

TCODColor Map::GetBackColor(const Coordinate& p) const { 
	if (Map::IsInside(p)) return tile(p).GetBackColor(); 
	return TCODColor::yellow;
}

void Map::SetNatureObject(const Coordinate& p, int val) { 
	if (Map::IsInside(p)) {
		tile(p).SetNatureObject(val);
	}
}
int Map::GetNatureObject(const Coordinate& p) const { 
	if (Map::IsInside(p)) return tile(p).GetNatureObject(); 
	return -1;
}

boost::weak_ptr<FilthNode> Map::GetFilth(const Coordinate& p) { 
	if (Map::IsInside(p)) return tile(p).GetFilth(); 
	return boost::weak_ptr<FilthNode>();
}
void Map::SetFilth(const Coordinate& p, boost::shared_ptr<FilthNode> value) { 
	if (Map::IsInside(p)) {
		tile(p).SetFilth(value);
		changedTiles.insert(p);
	}
}

boost::weak_ptr<BloodNode> Map::GetBlood(const Coordinate& p) { 
	if (Map::IsInside(p)) return tile(p).GetBlood(); 
	return boost::weak_ptr<BloodNode>();
}
void Map::SetBlood(const Coordinate& p, boost::shared_ptr<BloodNode> value) { 
	if (Map::IsInside(p)) tile(p).SetBlood(value); 
}

boost::weak_ptr<FireNode> Map::GetFire(const Coordinate& p) { 
	if (Map::IsInside(p)) return tile(p).GetFire(); 
	return boost::weak_ptr<FireNode>();
}
void Map::SetFire(const Coordinate& p, boost::shared_ptr<FireNode> value) { 
	if (Map::IsInside(p)) {
		tile(p).SetFire(value);
		changedTiles.insert(p);
	}
}

bool Map::BlocksLight(const Coordinate& p) const { 
	if (Map::IsInside(p)) return tile(p).BlocksLight(); 
	return true;
}
void Map::SetBlocksLight(const Coordinate& p, bool val) { 
	if (Map::IsInside(p)) tile(p).SetBlocksLight(val); 
}

bool Map::LineOfSight(const Coordinate& a, const Coordinate& b) {
	TCODLine::init(a.X(), a.Y(), b.X(), b.Y());
	int x = a.X();
	int y = a.Y();
	do {
		if (BlocksLight(Coordinate(x,y))) return false;
	} while(!TCODLine::step(&x, &y) && !(x == b.X() && y == b.Y()));
	return true;
}

void Map::Reset(const Coordinate& p) {
	if (Map::IsInside(p)) {
		tile(p).ResetType(TILEGRASS);
		tile(p).SetWalkable(true);
		tile(p).SetBuildable(true);
		tile(p).SetConstruction(-1);
		tile(p).SetWater(boost::shared_ptr<WaterNode>());
		tile(p).SetLow(false);
		tile(p).SetBlocksWater(false);
		tile(p).SetBlocksLight(false);
		tile(p).SetNatureObject(-1);
		tile(p).itemList.clear();
		tile(p).npcList.clear();
		tile(p).SetFilth(boost::shared_ptr<FilthNode>());
		tile(p).SetBlood(boost::shared_ptr<BloodNode>());
		tile(p).marked = false;
		tile(p).walkedOver = 0;
		tile(p).corruption = 0;
		tile(p).territory = false;
		tile(p).burnt = 0;
		tile(p).moveCost = 1;
		tile(p).flow = NODIRECTION;
		changedTiles.insert(p);
		heightMap->setValue(p.X(),p.Y(),0.5f);
	} else {
		waterlevel = -0.8f;
		overlayFlags = 0;
		mapMarkers.clear();
		markerids = 0;
		weather.reset(new Weather(this));
	}
}

void Map::Mark(const Coordinate& p) { tile(p).Mark(); }
void Map::Unmark(const Coordinate& p) { tile(p).Unmark(); }

int Map::GetMoveModifier(const Coordinate& p) {
	int modifier = 0;

	boost::shared_ptr<Construction> construction;
	if (tile(p).construction >= 0) construction = Game::Inst()->GetConstruction(tile(p).construction).lock();
	bool bridge = false;
	if (construction) bridge = (construction->Built() && construction->HasTag(BRIDGE));

	if (tile(p).GetType() == TILEBOG && !bridge) modifier += 10;
	else if (tile(p).GetType() == TILEDITCH && !bridge) modifier += 4;
	else if (tile(p).GetType() == TILEMUD && !bridge) { //Mud adds 6 if there's no bridge
		modifier += 6;
	}
	if (boost::shared_ptr<WaterNode> water = tile(p).GetWater().lock()) { //Water adds 'depth' without a bridge
		if (!bridge) modifier += water->Depth();
	}

	//Constructions (except bridges) slow down movement
	if (construction && !bridge) modifier += construction->GetMoveSpeedModifier();

	//Other critters slow down movement
	if (tile(p).npcList.size() > 0) modifier += 2 + Random::Generate(tile(p).npcList.size() - 1);

	return modifier;
}

float Map::GetWaterlevel() { return waterlevel; }

bool Map::GroundMarked(const Coordinate& p) { return tile(p).marked; }

void Map::WalkOver(const Coordinate& p) { if (Map::IsInside(p)) tile(p).WalkOver(); }

void Map::Corrupt(const Coordinate& pos, int magnitude) {
	int loops = 0;
	int x = pos.X(), y = pos.Y();
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
					!boost::iequals(Game::Inst()->natureList[tileMap[x][y].natureObject]->Name(),"Withering tree") &&
					!Game::Inst()->natureList[tileMap[x][y].natureObject]->IsIce()) {
						bool createTree = Game::Inst()->natureList[tileMap[x][y].natureObject]->Tree();
						Game::Inst()->RemoveNatureObject(Game::Inst()->natureList[tileMap[x][y].natureObject]);
						if (createTree && Random::Generate(6) < 1) Game::Inst()->CreateNatureObject(Coordinate(x,y), "Withering tree");
				}
			}
		}

		x += Random::Generate(-1, 1);
		y += Random::Generate(-1, 1);
	}
}

void Map::Naturify(const Coordinate& p) {
	if (Map::IsInside(p)) {
		if (tile(p).walkedOver > 0) --tile(p).walkedOver;
		if (tile(p).burnt > 0) tile(p).Burn(-1);
		if (tile(p).walkedOver == 0 && tile(p).natureObject < 0 && tile(p).construction < 0) {
			int natureObjects = 0;
			int width = extent.X(), height = extent.Y();
			int x = p.X(), y = p.Y();
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

int Map::GetCorruption(const Coordinate& p) { 
	if (Map::IsInside(p)) return tile(p).corruption;
	return 0;
}

bool Map::IsTerritory(const Coordinate& p) {
	return Map::IsInside(p) && tile(p).territory;
}

void Map::SetTerritory(const Coordinate& p, bool value) {
	if (Map::IsInside(p)) tile(p).territory = value;
}

void Map::SetTerritoryRectangle(const Coordinate& a, const Coordinate& b, bool value) {
	for (int x = a.X(); x <= b.X(); ++x) {
		for (int y = a.Y(); y <= b.Y(); ++y) {
			SetTerritory(Coordinate(x,y), value);
		}
	}
}

int Map::GetOverlayFlags() { return overlayFlags; }

void Map::AddOverlay(int flags) { overlayFlags |= flags; }
void Map::RemoveOverlay(int flags) { overlayFlags = overlayFlags & ~flags; }

void Map::ToggleOverlay(int flags) { overlayFlags ^= flags; }

void Map::FindEquivalentMoveTarget(const Coordinate& current, Coordinate& move, const Coordinate& next, void* npc) {
	int currentX, currentY, moveX, moveY, nextX, nextY;
	current.assign(&currentX, &currentY);
	//we will re-set move at the end
	current.assign(&moveX, &moveY);
	next.assign(&nextX, &nextY);

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
	if (right >= Width()) right = Width()-1;
	if (up < 0) up = 0;
	if (down >= Height()) down = Height()-1;

	Coordinate currentSafe(currentX, currentY), nextSafe(nextX, nextY);

	//Find a suitable target
	for (int x = left; x <= right; ++x) {
		for (int y = up; y <= down; ++y) {
			if (x != moveX || y != moveY) { //Only consider tiles not == moveX,moveY
				Coordinate xy(x,y);
				if (Map::IsWalkable(xy, npc) && tileMap[x][y].npcList.size() == 0 && !IsUnbridgedWater(xy) &&
					!IsDangerous(xy, static_cast<NPC*>(npc)->GetFaction())) {
					if (Game::Adjacent(xy, currentSafe) && Game::Adjacent(xy, move) && Game::Adjacent(xy, nextSafe)) {
						move.X(x);
						move.Y(y);
						return;
					}
				}
			}
		}
	}
}

bool Map::IsUnbridgedWater(const Coordinate& p) {
	if (Map::IsInside(p)) {
		if (boost::shared_ptr<WaterNode> water = tile(p).water) {
			boost::shared_ptr<Construction> construction = Game::Inst()->GetConstruction(tile(p).construction).lock();
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
		if (static_cast<int>(markeri->first) == markid) {
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

TCODColor Map::GetColor(const Coordinate& p) {
	if (Map::IsInside(p)) return tile(p).GetForeColor();
	return TCODColor::white;
}

void Map::Burn(const Coordinate& p, int magnitude) {
	if (Map::IsInside(p)) {
		tile(p).Burn(magnitude);
	}
}

int Map::Burnt(const Coordinate& p) {
	if (Map::IsInside(p)) {
		return tile(p).burnt;
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
				Coordinate pos(x,y);
				if (IsInside(pos)) {
					if (touched.find(pos) == touched.end() && tile(pos).water) {
							int distance = Distance(beginning, pos);
							touched.insert(pos);
							unfinished.push(std::pair<int, Coordinate>(INT_MAX - distance, pos));
							if (stage == 0 && distance > distance1) {
								stage = 1;
								favorA = false;
								favorB = false;
								if (std::abs(px[1] - px[2]) - std::abs(py[1] - py[2]) > 15)
									favorA = true;
								else if (std::abs(px[1] - px[2]) - std::abs(py[1] - py[2]) < 15)
									favorB = true;
							}
							if (stage == 1 && Distance(pos, Coordinate(px[1], py[1])) > distance2) {
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

        /* Calculate flow for all ground tiles

           'flow' is used for propagation of filth over time, and
           deplacement of objects in the river.

           Flow is determined by the heightmap: each tile flows to its
           lowest neighbor. When all neighbors have the same height,
           we choose to flow towards the river, by picking a random
           water tile and flowing toward it.

           For algorithmic reason, we build a "cache" of the water
           tiles list as a vector : picking a random water tile was
           O(N) with the water list, causing noticeable delays during
           map creation -- one minute or more -- while it is O(1) on
           water arrays.
         */
        std::vector<boost::weak_ptr<WaterNode> > waterArray(Game::Inst()->waterList.begin(), Game::Inst()->waterList.end());

	for (int y = 0; y < Height(); ++y) {
		for (int x = 0; x < Width(); ++x) {
			Coordinate pos(x,y);
			if (tile(pos).flow == NODIRECTION) {
				Coordinate lowest(x,y);
				for (int iy = y-1; iy <= y+1; ++iy) {
					for (int ix = x-1; ix <= x+1; ++ix) {
						Coordinate candidate(ix,iy);
						if (IsInside(candidate)) {
							if (heightMap->getValue(ix, iy) < heightMap->getValue(lowest.X(), lowest.Y())) {
								lowest = candidate;
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

				if (tileMap[x][y].flow == NODIRECTION && !waterArray.empty()) {
                                        /* No slope here, so approximate towards river

                                           (gasche) if there is no water, the tile will stay NODIRECTION. Is this
                                           the expected behavior? Should we assert that there always are some
                                           water tiles?
                                        */
					boost::weak_ptr<WaterNode> randomWater = Random::ChooseElement(waterArray);
					Coordinate coord = randomWater.lock()->Position();
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

Direction Map::GetFlow(const Coordinate& p) {
	if (Map::IsInside(p))
		return tile(p).flow;
	return NODIRECTION;
}

bool Map::IsDangerous(const Coordinate& p, int faction) const {
	if (Map::IsInside(p)) {
		if (tile(p).fire) return true;
		return Faction::factions[faction]->IsTrapVisible(p);
	}
	return false;
}

int Map::GetTerrainMoveCost(const Coordinate& p) const {
	if (Map::IsInside(p))
		return tile(p).GetTerrainMoveCost();
	return 0;
}

void Map::Update() {
	if (Random::Generate(UPDATES_PER_SECOND * 1) == 0) Naturify(Random::ChooseInExtent(Extent()));
	UpdateMarkers();
	weather->Update();
	UpdateCache();
}

//Finds a tile close to 'center' that will give an advantage to a creature with a ranged weapon
Coordinate Map::FindRangedAdvantage(const Coordinate& center) {
	std::vector<Coordinate> potentialPositions;
	for (int x = center.X() - 5; x <= center.X() + 5; ++x) {
		for (int y = center.Y() - 5; y <= center.Y() + 5; ++y) {
			if (IsInside(Coordinate(x,y))) {
				if (tileMap[x][y].construction >= 0 &&
					!tileMap[x][y].fire &&
					Game::Inst()->GetConstruction(tileMap[x][y].construction).lock() &&
					Game::Inst()->GetConstruction(tileMap[x][y].construction).lock()->HasTag(RANGEDADVANTAGE) &&
					tileMap[x][y].npcList.empty()) {
						potentialPositions.push_back(Coordinate(x,y));
				}
			}
		}
	}
	if (!potentialPositions.empty())
		return Random::ChooseElement(potentialPositions);
	return Coordinate(-1,-1);
}

void Map::UpdateCache() {
	boost::unique_lock<boost::shared_mutex> writeLock(cacheMutex);
	for (boost::unordered_set<Coordinate>::iterator tilei = changedTiles.begin(); tilei != changedTiles.end();) {
		cachedTile(*tilei) = tile(*tilei);
		tilei = changedTiles.erase(tilei);
	}
}

bool Map::IsDangerousCache(const Coordinate& p, int faction) const {
	return Map::IsInside(p) && (cachedTile(p).fire || Faction::factions[faction]->IsTrapVisible(p));
}

void Map::TileChanged(const Coordinate& p) {
	if (Map::IsInside(p)) {
		changedTiles.insert(p);
	}
}

void Map::save(OutputArchive& ar, const unsigned int version) const {
	for (size_t x = 0; x < tileMap.size(); ++x) {
		for (size_t y = 0; y < tileMap[x].size(); ++y) {
			ar & tileMap[x][y];
		}
	}
	ar & extent;
	ar & mapMarkers;
	ar & markerids;
	ar & weather;
	for (size_t x = 0; x < tileMap.size(); ++x) {
		for (size_t y = 0; y < tileMap[x].size(); ++y) {
			float heightMapValue = heightMap->getValue(x, y);
			ar & heightMapValue;
		}
	}
}

void Map::load(InputArchive& ar, const unsigned int version) {
	for (size_t x = 0; x < tileMap.size(); ++x) {
		for (size_t y = 0; y < tileMap[x].size(); ++y) {
			ar & tileMap[x][y];
		}
	}
	ar & extent;
	ar & mapMarkers;
	ar & markerids;
	if (version == 0) {
		Direction unused;
		ar & unused;
	}
	if (version >= 1) {
		ar & weather;
	}
	if (version >= 2) {
		for (size_t x = 0; x < tileMap.size(); ++x) {
			for (size_t y = 0; y < tileMap[x].size(); ++y) {
				float heightMapValue;
				ar & heightMapValue;
				heightMap->setValue(x, y, heightMapValue);
			}
		}
	}
}
