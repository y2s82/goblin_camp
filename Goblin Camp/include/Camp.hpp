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

#include <boost/serialization/serialization.hpp>

#include "Coordinate.hpp"

class Camp {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	Camp();
	static Camp* instance;
	double centerX, centerY;
	unsigned int buildingCount;
	bool locked;
	Coordinate lockedCenter;
	unsigned tier;
	std::string name;
	int workshops, farmplots, production;
	Coordinate upperCorner, lowerCorner;
	bool autoTerritory;

public:
	static Camp* Inst();
	Coordinate Center();
	void UpdateCenter(Coordinate, bool);
	void SetCenter(Coordinate);
	void LockCenter(Coordinate);
	void UnlockCenter();
	unsigned GetTier();
	void UpdateTier();
	std::string GetName();
	void ConstructionBuilt(int type);
	void ItemProduced();
	void DisableAutoTerritory();
	void ToggleAutoTerritory();
	bool IsAutoTerritoryEnabled();
	void Reset();
};
