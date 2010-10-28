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
#pragma once

#include "Construction.hpp"
#include "UI\Dialog.hpp"
#include "UI\UIComponents.hpp"

class SpawningPool : public Construction {
	friend class boost::serialization::access;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

	Dialog* dialog;
	UIContainer* container;
	bool dumpFilth, dumpCorpses;
	Coordinate a, b;
	unsigned int expansion, filth;
	boost::shared_ptr<Container> corpseContainer;
public:
	SpawningPool(ConstructionType = 0, Coordinate = Coordinate(0,0));
	Panel* GetContextMenu();
	static bool DumpFilth(SpawningPool*);
	static void ToggleDumpFilth(SpawningPool*);
	static bool DumpCorpses(SpawningPool*);
	static void ToggleDumpCorpses(SpawningPool*);
	void Update();
	void Draw(Coordinate, TCODConsole*);
	void Expand();
};
