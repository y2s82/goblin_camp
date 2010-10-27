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

Camp* Camp::instance = 0;

Camp::Camp() :
	centerX(220.0),
	centerY(220.0),
	buildingCount(0)
{}

Camp* Camp::Inst() {
	if (!instance) instance = new Camp();
	return instance;
}

Coordinate Camp::Center() {
	return Coordinate((int)centerX, (int)centerY);
}

void Camp::UpdateCenter(Coordinate newBuilding, bool add) {
	if(add) {
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