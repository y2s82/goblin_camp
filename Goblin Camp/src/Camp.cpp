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

#include "Camp.hpp"
#include "Coordinate.hpp"
#include "Game.hpp"
#include "Announce.hpp"
#include "scripting/Event.hpp"
#include "Random.hpp"
#include "JobManager.hpp"

Camp* Camp::instance = 0;

Camp::Camp() :
	centerX(220.0),
	centerY(220.0),
	buildingCount(0),
	locked(false),
	lockedCenter(0,0),
	tier(0),
	name("Clearing"),
	workshops(0),
	farmplots(0),
	production(0),
	upperCorner(Coordinate()),
	lowerCorner(Coordinate()),
	autoTerritory(true)
{}

Camp* Camp::Inst() {
	if (!instance) instance = new Camp();
	return instance;
}

Coordinate Camp::Center() {
	return locked ? lockedCenter : Coordinate((int)centerX, (int)centerY);
}

void Camp::UpdateCenter(Coordinate newBuilding, bool add) {
	if(add) {

		if (buildingCount == 0) {
			upperCorner = newBuilding;
			lowerCorner = newBuilding;
		} else {
			if (newBuilding.X() < upperCorner.X()) upperCorner.X(newBuilding.X());
			if (newBuilding.X() > lowerCorner.X()) lowerCorner.X(newBuilding.X());
			if (newBuilding.Y() < upperCorner.Y()) upperCorner.Y(newBuilding.Y());
			if (newBuilding.Y() > lowerCorner.Y()) lowerCorner.Y(newBuilding.Y());
			if (autoTerritory) Map::Inst()->SetTerritoryRectangle(upperCorner, lowerCorner, true);
		}

		centerX = (centerX * buildingCount + newBuilding.X()) / (buildingCount + 1);
		centerY = (centerY * buildingCount + newBuilding.Y()) / (buildingCount + 1);
		++buildingCount;
	} else {
		if(buildingCount > 1) {
			centerX = (centerX * buildingCount - newBuilding.X()) / (buildingCount - 1);
			centerY = (centerY * buildingCount - newBuilding.Y()) / (buildingCount - 1);
		} else {
			centerX = 220.0;
			centerY = 220.0;
		}
		--buildingCount;
	}
}

void Camp::SetCenter(Coordinate newCenter) {
	centerX = newCenter.X();
	centerY = newCenter.Y();
}

void Camp::LockCenter(Coordinate newCenter) {
	lockedCenter = newCenter;
	locked = true;
}

void Camp::UnlockCenter() { locked = false; }

int Camp::GetTier() { return tier; }

void Camp::UpdateTier() {
	
	int population = Game::Inst()->OrcCount() + Game::Inst()->GoblinCount();

	int newTier = tier;
	if (farmplots > 0 && workshops > 1 && production > 20 && population >= 20 && population < 30)
		newTier = 1;
	else if (workshops > 5 && production > 100 && population >= 30 && population < 40)
		newTier = 2;
	else if (workshops > 10 && production > 500 && population >= 40 && population < 60)
		newTier = 3;
	else if (production > 1000 && population >= 60 && population < 100)
		newTier = 4;
	else if (production > 3000 && population >= 100 && population < 200)
		newTier = 5;
	else if (production > 10000 && population >= 200)
		newTier = 6;

	if (newTier <= tier-2) ++newTier; //Only drop the camp tier down by one.
	else newTier = tier;

	if (newTier != tier) {
		bool positive = newTier > tier;
		tier = newTier;
		std::string oldName = name;
		if (tier == 0) {
			article = "a";
			name = "Clearing";
		} else if (tier == 1) {
			article = "a";
			name = "Camp";
		} else if (tier == 2) { 
			article = "a";
			name = "Settlement";
		} else if (tier == 3) {
			article = "an";
			name = "Outpost";
		} else if (tier == 4) {
			article = "a";
			name = "Fort";
		} else if (tier == 5) {
			article = "a";
			name = "Stronghold";
		} else if (tier == 6) {
			article = "a";
			name = "Citadel";
		}
	
		Announce::Inst()->AddMsg("Your "+oldName+" is now " + article + " " + name + "!", 
			positive ? TCODColor::lightGreen : TCODColor::yellow);
		Script::Event::TierChanged(tier, name);
	}
}

std::string Camp::GetName() { return name; }

void Camp::ConstructionBuilt(int type) {
	if (Construction::Presets[type].tags[WORKSHOP]) ++workshops;
	if (Construction::Presets[type].tags[FARMPLOT]) ++farmplots;
}

void Camp::ItemProduced() { ++production; }

void Camp::DisableAutoTerritory() { autoTerritory = false; }
void Camp::ToggleAutoTerritory() {
	autoTerritory = !autoTerritory;
	Announce::Inst()->AddMsg((boost::format("Automatic territory handling %s") % (autoTerritory ? "enabled" : "disabled")).str(), TCODColor::cyan);
}

bool Camp::IsAutoTerritoryEnabled() { return autoTerritory; }

void Camp::Reset() {
	centerX = 220.0;
	centerY = 220.0;
	buildingCount = 0;
	locked = false;
	lockedCenter = Coordinate(0,0);
	tier = 0;
	name = "Clearing";
	workshops = 0;
	farmplots = 0;
	production = 0;
	upperCorner = Coordinate(0, 0);
	lowerCorner = Coordinate(0, 0);
	autoTerritory = true;
	waterZones.clear();
	menialWaterJobs.clear();
	expertWaterJobs.clear();
}

void Camp::Update() {
	UpdateTier();
	UpdateWaterJobs();
}

void Camp::AddWaterZone(Coordinate from, Coordinate to) {
	for (int x = from.X(); x <= to.X(); ++x) {
		for (int y = from.Y(); y <= to.Y(); ++y) {
			if (x >= 0 && x < Map::Inst()->Width() && y >= 0 && y < Map::Inst()->Height()) {
				if (!Map::Inst()->GroundMarked(x, y)) {
					waterZones.insert(Coordinate(x,y));
					Map::Inst()->Mark(x,y);
				}
			}
		}
	}
}

void Camp::RemoveWaterZone(Coordinate from, Coordinate to) {
	for (int x = from.X(); x <= to.X(); ++x) {
		for (int y = from.Y(); y <= to.Y(); ++y) {
			if (x >= 0 && x < Map::Inst()->Width() && y >= 0 && y < Map::Inst()->Height()) {
				if (waterZones.find(Coordinate(x,y)) != waterZones.end()) {
					waterZones.erase(Coordinate(x,y));
					Map::Inst()->Unmark(x,y);
				}
			}
		}
	}
}

inline void CreateWaterJob(boost::shared_ptr<Job> waterJob, Coordinate location) {
	waterJob->SetRequiredTool(Item::StringToItemCategory("Bucket"));
	waterJob->Attempts(1);
	Coordinate waterLocation = Game::Inst()->FindWater(location);
	waterJob->tasks.push_back(Task(MOVEADJACENT, waterLocation));
	waterJob->tasks.push_back(Task(FILL, waterLocation));
	if (waterLocation.X() != -1 && waterLocation.Y() != -1) {
		waterJob->tasks.push_back(Task(MOVEADJACENT, location));
		waterJob->tasks.push_back(Task(POUR, location));
		waterJob->DisregardTerritory();
	} else {
		waterJob.reset();
	}
}

void Camp::UpdateWaterJobs() {

	//Remove finished jobs
	for (std::list<boost::weak_ptr<Job> >::iterator jobi = menialWaterJobs.begin(); jobi != menialWaterJobs.end();) {
		if (!jobi->lock()) jobi = menialWaterJobs.erase(jobi);
		else ++jobi;
	}
	for (std::list<boost::weak_ptr<Job> >::iterator jobi = expertWaterJobs.begin(); jobi != expertWaterJobs.end();) {
		if (!jobi->lock()) jobi = expertWaterJobs.erase(jobi);
		else ++jobi;
	}

	if (waterZones.size() > 0) {
		//The amount and priority of water pouring jobs depends on if there's fire anywhere
		if (Game::Inst()->fireList.size() > 0) {
			for (int i = 0; menialWaterJobs.size() < Game::Inst()->GoblinCount() && i < 10; ++i) {
				boost::shared_ptr<Job> waterJob(new Job("Pour water", HIGH, 0, true));
				Coordinate location = *boost::next(waterZones.begin(), Random::Generate(waterZones.size()-1));
				CreateWaterJob(waterJob, location);
				if (waterJob) {
					menialWaterJobs.push_back(waterJob);
					JobManager::Inst()->AddJob(waterJob);
				}
			}

			for (int i = 0; expertWaterJobs.size() < Game::Inst()->OrcCount() && i < 10; ++i) {
				boost::shared_ptr<Job> waterJob(new Job("Pour water", HIGH, 0, false));
				Coordinate location = *boost::next(waterZones.begin(), Random::Generate(waterZones.size()-1));
				CreateWaterJob(waterJob, location);
				if (waterJob) {
					expertWaterJobs.push_back(waterJob);
					JobManager::Inst()->AddJob(waterJob);
				}
			}

		} else {
			if (menialWaterJobs.size() < 5) {
				boost::shared_ptr<Job> waterJob(new Job("Pour water", LOW, 0, true));
				Coordinate location = *boost::next(waterZones.begin(), Random::Generate(waterZones.size()-1));
				CreateWaterJob(waterJob, location);
				if (waterJob) {
					menialWaterJobs.push_back(waterJob);
					JobManager::Inst()->AddJob(waterJob);
				}
			}
		}
	}
}
