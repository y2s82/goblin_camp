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

#include "Construction.hpp"
#include "UI/Dialog.hpp"
#include "UI/UIComponents.hpp"

#include "data/Serialization.hpp"

class SpawningPool : public Construction {
	GC_SERIALIZABLE_CLASS
	
	Dialog* dialog;
	UIContainer* container;
	bool dumpFilth, dumpCorpses;
	Coordinate a, b;
	unsigned int expansion, filth, corpses, spawns;
	unsigned int expansionLeft, corruptionLeft, spawnsLeft;
	boost::shared_ptr<Container> corpseContainer;
	int jobCount;
	int burn;
public:
	SpawningPool(ConstructionType = 0, const Coordinate& = zero);
	Panel* GetContextMenu();
	static bool DumpFilth(SpawningPool*);
	static void ToggleDumpFilth(SpawningPool*);
	static bool DumpCorpses(SpawningPool*);
	static void ToggleDumpCorpses(SpawningPool*);
	void Update();
	void Draw(Coordinate, TCODConsole*);
	void Expand(bool message=true);
	virtual void CancelJob(int=0);
	virtual void AcceptVisitor(ConstructionVisitor& visitor);
	void Burn();
	virtual int Build();
	boost::shared_ptr<Container>& GetContainer();
	void Spawn();
private:
	Coordinate SpawnLocation();
};

BOOST_CLASS_VERSION(SpawningPool, 0)
