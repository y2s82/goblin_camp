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

#include "Camp.hpp"
#include "Coordinate.hpp"
#include "Game.hpp"
#include "Announce.hpp"
#include "scripting/Event.hpp"

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

unsigned Camp::GetTier() { return tier; }

void Camp::UpdateTier() {
	unsigned oldTier = tier;
	
	switch (tier) {
	case 0:
		if (farmplots > 0 && workshops > 1 && production > 20 && Game::Inst()->OrcCount() + Game::Inst()->GoblinCount() > 20)
			++tier;
		break;

	case 1:
		if (workshops > 5 && production > 100 && Game::Inst()->OrcCount() + Game::Inst()->GoblinCount() > 30)
			++tier;
		break;
	case 2: 
		if (workshops > 10 && production > 500 && Game::Inst()->OrcCount() + Game::Inst()->GoblinCount() > 40)
			++tier;
		break;
	case 3: 
		if (production > 1000 && Game::Inst()->OrcCount() + Game::Inst()->GoblinCount() > 60)
			++tier;
		break;
	case 4: 
		if (production > 3000 && Game::Inst()->OrcCount() + Game::Inst()->GoblinCount() > 100)
			++tier;
		break;
	case 5: 
		if (production > 10000 && Game::Inst()->OrcCount() + Game::Inst()->GoblinCount() > 200)
			++tier;
		break;
	case 6: break;
	default: tier = 0; break;
	}

	if (tier == 0) name = "a Clearing";
	else if (tier == 1) name = "a Camp";
	else if (tier == 2) name = "a Settlement";
	else if (tier == 3) name = "an Outpost";
	else if (tier == 4) name = "a Fort";
	else if (tier == 5) name = "a Stronghold";
	else if (tier == 6) name = "a Citadel";
	
	if (tier != oldTier) {
		Announce::Inst()->AddMsg("Your camp advanced to a higher tier and is now " + name + "!", TCODColor::lightGreen);
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
}
