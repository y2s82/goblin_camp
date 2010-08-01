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
center(Coordinate(220,220)),
	buildingCount(0)
{}

Camp* Camp::Inst() {
	if (!instance) instance = new Camp();
	return instance;
}

Coordinate Camp::Center() {
	return center;
}

void Camp::UpdateCenter(Coordinate newBuilding) {
	++buildingCount;
	xAcc(newBuilding.X(),boost::accumulators::weight = 1);
	yAcc(newBuilding.Y(),boost::accumulators::weight = 1);
	center = Coordinate((int)boost::accumulators::mean(xAcc), (int)boost::accumulators::mean(yAcc));
}