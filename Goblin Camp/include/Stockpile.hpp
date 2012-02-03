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

#include "Coordinate.hpp"
#include "Construction.hpp"
#include "Container.hpp"
#include "data/Serialization.hpp"

class Item;

#define NOTFULL (1 << 0)
#define BETTERTHAN (1 << 1)
#define APPLYMINIMUMS (1 << 2)
#define EMPTY (1 << 3)
#define MOSTDECAYED (1 << 4)
#define AVOIDGARBAGE (1 << 5)

class Stockpile : public Construction, public ContainerListener {
	GC_SERIALIZABLE_CLASS
	
	friend class Game;
protected:
	Stockpile(ConstructionType=0, int symbol=0, Coordinate=Coordinate(0,0));

	int symbol;
	Coordinate a, b; /*Opposite corners so we know which tiles the stockpile
					 approximately encompasses*/
	int capacity;
	std::map<ItemCategory, int> amount;
	std::map<ItemCategory, bool> allowed;
	std::map<Coordinate, bool> reserved;
	std::map<Coordinate, boost::shared_ptr<Container> > containers;
	std::map<Coordinate, TCODColor> colors;
	std::map<ItemCategory, int> limits;
	std::map<ItemCategory, int> demand;
	std::map<ItemCategory, int> lastDemandBalance; //At what amount did we last check container demand?
public:
	virtual ~Stockpile();
	int Build();
	virtual void Draw(Coordinate, TCODConsole*);
	boost::weak_ptr<Item> FindItemByCategory(ItemCategory, int flags=0, int value=0);
	boost::weak_ptr<Item> FindItemByType(ItemType, int flags=0, int value=0);
	int Symbol();
	void Symbol(int);
	int Expand(Coordinate,Coordinate);
	bool Allowed(ItemCategory);
	bool Allowed(std::set<ItemCategory>);
	virtual bool Full(ItemType = -1);
	virtual Coordinate FreePosition();
	void ReserveSpot(Coordinate, bool, ItemType);
	boost::weak_ptr<Container> Storage(Coordinate);
	void SwitchAllowed(ItemCategory, bool childrenAlso = true, bool countParentsOnly = false);
	void SetAllAllowed(bool);
	virtual void GetTooltip(int x, int y, Tooltip *tooltip);
	void ItemAdded(boost::weak_ptr<Item>);
	void ItemRemoved(boost::weak_ptr<Item>);
	Coordinate Center();
	void TranslateInternalContainerListeners();
	void AdjustLimit(ItemCategory, int);
	int GetLimit(ItemCategory);
	virtual void AcceptVisitor(ConstructionVisitor& visitor);
	virtual void Dismantle(const Coordinate& pos = undefined);
	int GetDemand(ItemCategory);
	int GetAmount(ItemCategory);
	void Reorganize();
	virtual void SetMap(Map* map);
private:
	void Erase(const Coordinate&);
};

//1 = v0.2
BOOST_CLASS_VERSION(Stockpile, 1)
