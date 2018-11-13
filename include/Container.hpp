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

#include <set>

#include <boost/weak_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "Item.hpp"
#include "data/Serialization.hpp"

class ContainerListener {
public:
	virtual void ItemAdded(boost::weak_ptr<Item>) = 0;
	virtual void ItemRemoved(boost::weak_ptr<Item>) = 0;
};

class Container : public Item {
	GC_SERIALIZABLE_CLASS
	
	std::set<boost::weak_ptr<Item> > items;
	int capacity;
	int reservedSpace;
	
	std::vector<ContainerListener*> listeners;
	std::vector<int> listenersAsUids;

	int water, filth; //Special cases for real liquids
public:
	Container(Coordinate = Coordinate(0,0), ItemType type=0, int cap=1000, int faction = 0,
		std::vector<boost::weak_ptr<Item> > = std::vector<boost::weak_ptr<Item> >(),
		std::vector<ContainerListener*> = std::vector<ContainerListener*>());
	virtual ~Container();
	virtual bool AddItem(boost::weak_ptr<Item>);
	virtual void RemoveItem(boost::weak_ptr<Item>);
	void ReserveSpace(bool, int bulk = 1);
	boost::weak_ptr<Item> GetItem(boost::weak_ptr<Item>);
	std::set<boost::weak_ptr<Item> >* GetItems();
	boost::weak_ptr<Item> GetFirstItem();
	bool empty();
	int size();
	int Capacity();
	bool Full();
	std::set<boost::weak_ptr<Item> >::iterator begin();
	std::set<boost::weak_ptr<Item> >::iterator end();
	void AddListener(ContainerListener* listener);
	void RemoveListener(ContainerListener *listener);
	void GetTooltip(int x, int y, Tooltip *tooltip);
	void TranslateContainerListeners();
	void AddWater(int);
	void RemoveWater(int);
	int ContainsWater();
	void AddFilth(int);
	void RemoveFilth(int);
	int ContainsFilth();
	void Draw(Coordinate, TCODConsole*);
	int GetReservedSpace();
	virtual void Position(const Coordinate&);
	virtual Coordinate Position();
	virtual void SetFaction(int);
};

BOOST_CLASS_VERSION(::Container, 0)
