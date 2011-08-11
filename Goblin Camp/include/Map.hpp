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
#pragma once

#include <utility>
#include <list>

#include <boost/thread/shared_mutex.hpp>
#include <boost/multi_array.hpp>
#include <boost/unordered_set.hpp>
#include <libtcod.hpp>

#include "Tile.hpp"
#include "Coordinate.hpp"
#include "data/Serialization.hpp"

class MapMarker;
class Weather;

#define TERRITORY_OVERLAY (1 << 0)
#define TERRAIN_OVERLAY (2 << 0)

class Map : public ITCODPathCallback {
	GC_SERIALIZABLE_CLASS
	
	Map();
	static Map* instance;
	boost::multi_array<Tile, 2> tileMap;
	boost::multi_array<CacheTile, 2> cachedTileMap;
	Coordinate extent; //X->width, Y->height
	float waterlevel;
	int overlayFlags;
	std::list< std::pair<unsigned int, MapMarker> > mapMarkers;
	unsigned int markerids;
	boost::unordered_set<Coordinate> changedTiles;

	inline const Tile& tile(const Coordinate& p) const {
		return tileMap[p.X()][p.Y()];
	}
	inline const CacheTile& cachedTile(const Coordinate& p) const {
		return cachedTileMap[p.X()][p.Y()];
	}
	inline Tile& tile(const Coordinate& p) {
		return tileMap[p.X()][p.Y()];
	}
	inline CacheTile& cachedTile(const Coordinate& p) {
		return cachedTileMap[p.X()][p.Y()];
	}
	
public:
	typedef std::list<std::pair<unsigned int, MapMarker> >::const_iterator MarkerIterator;

	TCODHeightMap *heightMap;
	static Map* Inst();
	~Map();
	static void Reset();
	float getWalkCost(const Coordinate&, const Coordinate&, void *) const;
	float getWalkCost(int, int, int, int, void *) const;
	//we must keep the int-version of getWalkCost as an override of ITCODPathCallback, otherwise Map is virtual

	bool IsWalkable(const Coordinate&) const;
	bool IsWalkable(const Coordinate&,void*) const;
	void SetWalkable(const Coordinate&,bool);
	
	Coordinate Extent();
	int Width();
	int Height();

	bool IsInside(const Coordinate&) const;
	Coordinate Shrink(const Coordinate&) const;

	bool IsBuildable(const Coordinate&) const;
	void SetBuildable(const Coordinate&,bool);
	TileType GetType(const Coordinate&);
	void ResetType(const Coordinate&,TileType,float tileHeight = 0.0);  //ResetType() resets all tile variables to defaults
	void ChangeType(const Coordinate&,TileType,float tileHeight = 0.0); //ChangeType() preserves information such as buildability
	void MoveTo(const Coordinate&,int);
	void MoveFrom(const Coordinate&,int);
	void SetConstruction(const Coordinate&,int);
	int GetConstruction(const Coordinate&) const;
	boost::weak_ptr<WaterNode> GetWater(const Coordinate&);
	void SetWater(const Coordinate&,boost::shared_ptr<WaterNode>);
	bool IsLow(const Coordinate&) const;
	void SetLow(const Coordinate&,bool);
	bool BlocksWater(const Coordinate&) const;
	void SetBlocksWater(const Coordinate&,bool);
	std::set<int>* NPCList(const Coordinate&);
	int GetGraphic(const Coordinate&) const;
	TCODColor GetForeColor(const Coordinate&) const;
	void ForeColor(const Coordinate&,TCODColor);
	TCODColor GetBackColor(const Coordinate&) const;
	void SetNatureObject(const Coordinate&,int);
	int GetNatureObject(const Coordinate&) const;
	std::set<int>* ItemList(const Coordinate&);
	boost::weak_ptr<FilthNode> GetFilth(const Coordinate&);
	void SetFilth(const Coordinate&,boost::shared_ptr<FilthNode>);
	boost::weak_ptr<BloodNode> GetBlood(const Coordinate&);
	void SetBlood(const Coordinate&,boost::shared_ptr<BloodNode>);
	boost::weak_ptr<FireNode> GetFire(const Coordinate&);
	void SetFire(const Coordinate&,boost::shared_ptr<FireNode>);
	bool BlocksLight(const Coordinate&) const;
	void SetBlocksLight(const Coordinate&, bool);
	bool LineOfSight(const Coordinate&, const Coordinate&);
	void Mark(const Coordinate&);
	void Unmark(const Coordinate&);
	bool GroundMarked(const Coordinate&);
	int GetMoveModifier(const Coordinate&);
	float GetWaterlevel();
	void WalkOver(const Coordinate&);
	void Naturify(const Coordinate&);
	void Corrupt(const Coordinate&, int magnitude=200);
	int GetCorruption(const Coordinate&);
	bool IsTerritory(const Coordinate&);
	void SetTerritory(const Coordinate&, bool);
	void SetTerritoryRectangle(const Coordinate &high, const Coordinate &low, bool);
	int GetOverlayFlags();
	void AddOverlay(int flags);
	void RemoveOverlay(int flags);
	void ToggleOverlay(int flags);
	void FindEquivalentMoveTarget(const Coordinate& current, Coordinate& move, const Coordinate& next, void* npc); //mutates 'move'
	bool IsUnbridgedWater(const Coordinate&);
	void UpdateMarkers();
	unsigned int AddMarker(MapMarker);
	void RemoveMarker(int);
	TCODColor GetColor(const Coordinate&);
	void Burn(const Coordinate&, int magnitude=1);
	int Burnt(const Coordinate&);

	MarkerIterator MarkerBegin();
	MarkerIterator MarkerEnd();

	Direction GetWindDirection();
	void RandomizeWind();
	void ShiftWind();
	std::string GetWindAbbreviation();

	void CalculateFlow(int px[4], int py[4]); //TODO use Coordinate
	Direction GetFlow(const Coordinate&);

	bool IsDangerous(const Coordinate&, int faction) const;
	bool IsDangerousCache(const Coordinate&, int faction) const;
	int GetTerrainMoveCost(const Coordinate&) const;
	boost::shared_ptr<Weather> weather;
	void Update();
	
	Coordinate FindRangedAdvantage(const Coordinate&);

	mutable boost::shared_mutex cacheMutex;
	void UpdateCache();
	void TileChanged(const Coordinate&);
};

BOOST_CLASS_VERSION(Map, 2)
