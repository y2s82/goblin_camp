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
#include<memory>
#include "stdafx.hpp"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/weak_ptr.hpp>

#include "Entity.hpp"
#include "Container.hpp"
#include "Logger.hpp"
#include "Construction.hpp"
#include "Game.hpp"
#include "Stockpile.hpp"

Container::Container(
	Coordinate pos, ItemType type, int capValue, int faction, std::vector<std::weak_ptr<Item> > components,
	std::vector<ContainerListener*> nlisteners
) :
	Item(pos, type, faction, components),
	capacity(capValue),
	reservedSpace(0),
	listeners(nlisteners),
	water(0),
	filth(0)
{
}

Container::~Container() {
	while (!items.empty()) {
		std::weak_ptr<Item> item = GetFirstItem();
		RemoveItem(item);
		if (item.lock()) item.lock()->PutInContainer();
	}
}

bool Container::AddItem(std::weak_ptr<Item> witem) {
	std::shared_ptr<Item> item = witem.lock();
	if (item && capacity >= std::max(item->GetBulk(), 1)) {
		item->PutInContainer(std::static_pointer_cast<Item>(shared_from_this()));
		items.insert(item);
		capacity -= std::max(item->GetBulk(), 1); //<- so that bulk=0 items take space
		for(std::vector<ContainerListener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
			(*it)->ItemAdded(item);
		}
		if (item->Type() == Item::StringToItemType("water")) ++water;
		return true;
	}
	return false;
}


void Container::RemoveItem(std::weak_ptr<Item> item) {
	if (items.find(item) != items.end()) {
		items.erase(item);
		if (item.lock()) {
			capacity += std::max(item.lock()->GetBulk(), 1);
			if (item.lock()->Type() == Item::StringToItemType("water")) --water;
		}
		for(std::vector<ContainerListener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
			(*it)->ItemRemoved(item);
		}
	}
}

std::weak_ptr<Item> Container::GetItem(std::weak_ptr<Item> item) {
	return *items.find(item);
}

std::set<std::weak_ptr<Item> >* Container::GetItems() { return &items; }

bool Container::empty() { return items.empty(); }
int Container::size() { return items.size(); }

int Container::Capacity() { return capacity-reservedSpace; }

std::weak_ptr<Item> Container::GetFirstItem() { 
	if (items.empty()) return std::weak_ptr<Item>();
	return *items.begin(); 
}

std::set<std::weak_ptr<Item> >::iterator Container::begin() { return items.begin(); }
std::set<std::weak_ptr<Item> >::iterator Container::end() { return items.end(); }
bool Container::Full() {
	return (capacity-reservedSpace <= 0);
}

void Container::ReserveSpace(bool res, int bulk) {
	if (res) reservedSpace += std::max(1, bulk);
	else reservedSpace -= std::max(1,bulk);
}

void Container::AddListener(ContainerListener* listener) {
	listeners.push_back(listener);
	if (dynamic_cast<Stockpile*>(listener)) {
		listenersAsUids.push_back(dynamic_cast<Stockpile*>(listener)->Uid());
	}
}

void Container::RemoveListener(ContainerListener *listener) {
	unsigned int n = 0;
	for(std::vector<ContainerListener*>::iterator it = listeners.begin(); it != listeners.end(); it++, ++n) {
		if(*it == listener) {
			listeners.erase(it);
			if (n < listenersAsUids.size()) listenersAsUids.erase(std::next(listenersAsUids.begin(), n));
			return;
		}
	}
}

void Container::GetTooltip(int x, int y, Tooltip *tooltip) {
	int capacityUsed = 0;
	for (std::set<std::weak_ptr<Item> >::iterator itemi = items.begin(); itemi != items.end(); ++itemi) {
		if (itemi->lock()) capacityUsed += std::max(1, itemi->lock()->GetBulk());
	}
	tooltip->AddEntry(TooltipEntry(name + " - " + std::to_string(size()) + " items (" + std::to_string(capacityUsed)  + "/" + std::to_string(capacity + capacityUsed), TCODColor::white));
}

void Container::TranslateContainerListeners() {
	listeners.clear();
	for (unsigned int i = 0; i < listenersAsUids.size(); ++i) {
		std::weak_ptr<Construction> cons = Game::Inst()->GetConstruction(listenersAsUids[i]);
		if (cons.lock() && std::dynamic_pointer_cast<Stockpile>(cons.lock())) {
			listeners.push_back(std::dynamic_pointer_cast<Stockpile>(cons.lock()).get());
		}
	}
}

void Container::AddWater(int amount) {
	if (empty() && filth == 0) { 
		for (int i = 0; i < amount; ++i) {
			int waterUid = Game::Inst()->CreateItem(Position(), Item::StringToItemType("Water"));
			std::shared_ptr<Item> waterItem = Game::Inst()->GetItem(waterUid).lock();
			
			if (!AddItem(waterItem)) {
				Game::Inst()->RemoveItem(waterItem);
				break;
			}
		}
	}
}

void Container::RemoveWater(int amount) {
	for (int i = 0; i < amount; ++i) {
		for (std::set<std::weak_ptr<Item> >::iterator itemi = items.begin(); itemi != items.end(); ++itemi) {
			std::shared_ptr<Item> waterItem = itemi->lock();
			if (waterItem && waterItem->Type() == Item::StringToItemType("water")) {
				Game::Inst()->RemoveItem(waterItem);
				break;
			}
		}
	}
}

int Container::ContainsWater() { return water; }

void Container::AddFilth(int amount) {
	if (empty() && water == 0) filth += amount;
}

void Container::RemoveFilth(int amount) {
	filth -= amount;
	if (filth < 0) filth = 0;
}

int Container::ContainsFilth() { return filth; }

void Container::Draw(Coordinate upleft, TCODConsole* console) {
	int screenx = (pos - upleft).X();
	int screeny = (pos - upleft).Y();
	if (screenx >= 0 && screenx < console->getWidth() && screeny >= 0 && screeny < console->getHeight()) {
		if (!items.empty() && items.begin()->lock())
			console->putCharEx(screenx, screeny, items.begin()->lock()->GetGraphic(), items.begin()->lock()->Color(), color);
		else
			console->putCharEx(screenx, screeny, graphic, color, Map::Inst()->GetBackColor(pos));
	}
}

int Container::GetReservedSpace() { return reservedSpace; }

void Container::Position(const Coordinate& pos) {
	Item::Position(pos);
	for (std::set<std::weak_ptr<Item> >::iterator itemi = items.begin(); itemi != items.end(); ++itemi) {
		std::shared_ptr<Item> item = itemi->lock();
		if (item) item->Position(pos);
	}
}

Coordinate Container::Position() {return Item::Position();}

void Container::SetFaction(int faction) {
	for (std::set<std::weak_ptr<Item> >::const_iterator itemi = items.begin(); itemi != items.end(); ++itemi) {
		if (std::shared_ptr<Item> item = itemi->lock()) {
			item->SetFaction(faction);
		}
	}
	Item::SetFaction(faction);
}

void Container::save(OutputArchive& ar, const unsigned int version) const {
	ar & boost::serialization::base_object<Item>(*this);
	ar & items;
	ar & capacity;
	ar & reservedSpace;
	ar & listenersAsUids;
	ar & water;
	ar & filth;
}

void Container::load(InputArchive& ar, const unsigned int version) {
	ar & boost::serialization::base_object<Item>(*this);
	ar & items;
	ar & capacity;
	ar & reservedSpace;
	ar & listenersAsUids;
	ar & water;
	ar & filth;
}
