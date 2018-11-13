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

#include <boost/serialization/set.hpp>
#include <boost/serialization/list.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "Camp.hpp"
#include "Coordinate.hpp"
#include "Game.hpp"
#include "Announce.hpp"
#include "scripting/Event.hpp"
#include "Random.hpp"
#include "JobManager.hpp"
#include "Stats.hpp"
#include "Faction.hpp"
#include "SpawningPool.hpp"

Camp* Camp::instance = 0;

Camp::Camp() :
centerX(220.0),
	centerY(220.0),
	buildingCount(0),
	locked(false),
	lockedCenter(0,0),
	tier(0),
	name("Clearing"),
	article("a"),
	workshops(0),
	farmplots(0),
	upperCorner(Coordinate(0,0)),
	lowerCorner(Coordinate(0,0)),
	autoTerritory(true),
	diseaseModifier(0)
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

	// Immortius: There was previously an issue with new tier calculation - because
	// the check for each tier had a population range, if you exceeded that population
	// range before you achieved the other requirements you would not be able to reach that tier.
	// The solution is to check eligability from highest tier downwards and avoid population ranges.
	int newTier = 0;
	if (farmplots > 0)
	{
		if      (workshops > 10 && Stats::Inst()->GetItemsBuilt() > 10000 && population >= 200)
			newTier = 6;
		else if (workshops > 10 && Stats::Inst()->GetItemsBuilt() > 5000  && population >= 100)
			newTier = 5;
		else if (workshops > 10 && Stats::Inst()->GetItemsBuilt() > 3000  && population >= 70)
			newTier = 4;
		else if (workshops > 10 && Stats::Inst()->GetItemsBuilt() > 1000   && population >= 50)
			newTier = 3;
		else if (workshops > 5  && Stats::Inst()->GetItemsBuilt() > 400   && population >= 30)
			newTier = 2;
		else if (workshops > 1  && Stats::Inst()->GetItemsBuilt() > 20    && population >= 20)
			newTier = 1;
	}

	if (newTier < tier) ++newTier; //Only drop the camp tier down if newtier <= tier-2

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

void Camp::DisableAutoTerritory() { autoTerritory = false; }
void Camp::ToggleAutoTerritory() {
	autoTerritory = !autoTerritory;
	Announce::Inst()->AddMsg((boost::format("Automatic territory handling %s") % (autoTerritory ? "enabled" : "disabled")).str(), TCODColor::cyan);
}

bool Camp::IsAutoTerritoryEnabled() { return autoTerritory; }

void Camp::Reset() {
	delete instance;
	instance = 0;
}

void Camp::Update() {
	UpdateTier();
	UpdateWaterJobs();

	diseaseModifier = static_cast<int>(std::pow(static_cast<float>((Game::Inst()->GoblinCount() + Game::Inst()->OrcCount() - 20) / 10), 2.0f));
}

void Camp::AddWaterZone(Coordinate from, Coordinate to) {
	for (int x = from.X(); x <= to.X(); ++x) {
		for (int y = from.Y(); y <= to.Y(); ++y) {
			Coordinate p(x,y);
			if (Map::Inst()->IsInside(p)) {
				if (!Map::Inst()->GroundMarked(p)) {
					waterZones.insert(Coordinate(p));
					Map::Inst()->Mark(p);
				}
			}
		}
	}
}

void Camp::RemoveWaterZone(Coordinate from, Coordinate to) {
	for (int x = from.X(); x <= to.X(); ++x) {
		for (int y = from.Y(); y <= to.Y(); ++y) {
			Coordinate p(x,y);
			if (Map::Inst()->IsInside(p)) {
				if (waterZones.find(Coordinate(p)) != waterZones.end()) {
					waterZones.erase(Coordinate(p));
					Map::Inst()->Unmark(p);
				}
			}
		}
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
			for (int i = 1; static_cast<int>(menialWaterJobs.size()) < Game::Inst()->GoblinCount() && i <= 10; ++i) {
				boost::shared_ptr<Job> waterJob(new Job("Pour water", VERYHIGH, 0, true));
				Coordinate location = *boost::next(waterZones.begin(), Random::Generate(waterZones.size()-1));
				Job::CreatePourWaterJob(waterJob, location);
				if (waterJob) {
					menialWaterJobs.push_back(waterJob);
					JobManager::Inst()->AddJob(waterJob);
				}
			}

			for (int i = 1; static_cast<int>(expertWaterJobs.size()) < Game::Inst()->OrcCount() && i <= 10; ++i) {
				boost::shared_ptr<Job> waterJob(new Job("Pour water", VERYHIGH, 0, false));
				Coordinate location = *boost::next(waterZones.begin(), Random::Generate(waterZones.size()-1));
				Job::CreatePourWaterJob(waterJob, location);
				if (waterJob) {
					expertWaterJobs.push_back(waterJob);
					JobManager::Inst()->AddJob(waterJob);
				}
			}

		} else {
			if (menialWaterJobs.size() < 5) {
				boost::shared_ptr<Job> waterJob(new Job("Pour water", LOW, 0, true));
				Coordinate location = *boost::next(waterZones.begin(), Random::Generate(waterZones.size()-1));
				Job::CreatePourWaterJob(waterJob, location);
				if (waterJob) {
					menialWaterJobs.push_back(waterJob);
					JobManager::Inst()->AddJob(waterJob);
				}
			}
		}
	}
}

Coordinate Camp::GetUprTerritoryCorner() const { return upperCorner; }
Coordinate Camp::GetLowTerritoryCorner() const { return lowerCorner; }

Coordinate Camp::GetRandomSpot() const {
	for (int tries = 0; tries < 20; ++tries) {
		Coordinate randomLocation = Random::ChooseInRectangle(upperCorner, lowerCorner);		
		if (Map::Inst()->IsTerritory(randomLocation) &&
			!Map::Inst()->IsDangerous(randomLocation, PLAYERFACTION) &&
			Map::Inst()->IsWalkable(randomLocation) &&
			!Map::Inst()->GetWater(randomLocation).lock())
			return randomLocation;
	}

	return undefined;
}

int Camp::GetDiseaseModifier() {
	return diseaseModifier;
}

void Camp::save(OutputArchive& ar, const unsigned int version) const {
	ar.register_type<Coordinate>();
	ar & centerX;
	ar & centerY;
	ar & buildingCount;
	ar & locked;
	ar & lockedCenter;
	ar & tier;
	ar & name;
	ar & workshops;
	ar & farmplots;
	ar & upperCorner;
	ar & lowerCorner;
	ar & autoTerritory;
	ar & article;
	ar & waterZones;
	ar & menialWaterJobs;
	ar & expertWaterJobs;
	ar & spawningPool;
	ar & diseaseModifier;
}

void Camp::load(InputArchive& ar, const unsigned int version) {
	ar.register_type<Coordinate>();
	ar & centerX;
	ar & centerY;
	ar & buildingCount;
	ar & locked;
	ar & lockedCenter;
	ar & tier;
	ar & name;
	ar & workshops;
	ar & farmplots;
	if (version == 0) {
		int unused;
		ar & unused;
	}
	ar & upperCorner;
	ar & lowerCorner;
	ar & autoTerritory;
	ar & article;
	ar & waterZones;
	ar & menialWaterJobs;
	ar & expertWaterJobs;
	if (version >= 1) {
		ar & spawningPool;
	}
	if (version >= 2) {
		ar & diseaseModifier;
	}
}
