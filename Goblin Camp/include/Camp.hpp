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

#include <string>
#include <set>
#include <list>

#include <boost/weak_ptr.hpp>

#include "Coordinate.hpp"
#include "data/Serialization.hpp"

class Job;
class SpawningPool;

class Camp {
	GC_SERIALIZABLE_CLASS
	
	Camp();
	static Camp* instance;
	double centerX, centerY;
	unsigned int buildingCount;
	bool locked;
	Coordinate lockedCenter;
	int tier;
	std::string name;
	std::string article;
	int workshops, farmplots;
	Coordinate upperCorner, lowerCorner;
	bool autoTerritory;
	std::set<Coordinate> waterZones;
	std::list<boost::weak_ptr<Job> > menialWaterJobs;
	std::list<boost::weak_ptr<Job> > expertWaterJobs;
	int diseaseModifier;
public:
	static Camp* Inst();
	Coordinate Center();
	void UpdateCenter(Coordinate, bool);
	void SetCenter(Coordinate);
	void LockCenter(Coordinate);
	void UnlockCenter();
	int GetTier();
	void UpdateTier();
	std::string GetName();
	void ConstructionBuilt(int type);
	void DisableAutoTerritory();
	void ToggleAutoTerritory();
	bool IsAutoTerritoryEnabled();
	static void Reset();
	void Update();
	void AddWaterZone(Coordinate from, Coordinate to);
	void RemoveWaterZone(Coordinate from, Coordinate to);
	void UpdateWaterJobs();
	Coordinate GetUprTerritoryCorner() const;
	Coordinate GetLowTerritoryCorner() const;
	Coordinate GetRandomSpot() const;
	boost::weak_ptr<SpawningPool> spawningPool;
	int GetDiseaseModifier();
};

BOOST_CLASS_VERSION(Camp, 2)

//Version 2 = 0.2 - diseaseChance
