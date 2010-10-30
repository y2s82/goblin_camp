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

#include "Entity.hpp"
#include "Container.hpp"
#include "Logger.hpp"
#include "Construction.hpp"
#include "Game.hpp"

Container::Container(
	Coordinate pos, ItemType type, int capValue, int faction, std::vector<boost::weak_ptr<Item> > components,
	std::vector<ContainerListener*> nlisteners
) :
	Item(pos, type, faction, components),
	capacity(capValue),
	reservedSpace(0),
	listeners(nlisteners)
{
}

Container::~Container() {
	for (std::set<boost::weak_ptr<Item> >::iterator itemi = items.begin(); itemi != items.end(); ++itemi) {
		if (itemi->lock()) {
			itemi->lock()->PutInContainer(container);
		}
	}
}

bool Container::AddItem(boost::weak_ptr<Item> item) {
	if (capacity > 0 && item.lock()) {
		item.lock()->PutInContainer(boost::static_pointer_cast<Item>(shared_from_this()));
		items.insert(item);
		--capacity;
		for(std::vector<ContainerListener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
			(*it)->ItemAdded(item);
		}
		return true;
	}
	return false;
}

void Container::RemoveItem(boost::weak_ptr<Item> item) {
	items.erase(item);
	++capacity;
	for(std::vector<ContainerListener*>::iterator it = listeners.begin(); it != listeners.end(); it++) {
		(*it)->ItemRemoved(item);
	}
}

boost::weak_ptr<Item> Container::GetItem(boost::weak_ptr<Item> item) {
	return *items.find(item);
}

std::set<boost::weak_ptr<Item> >* Container::GetItems() { return &items; }

bool Container::empty() { return items.empty(); }
int Container::size() { return items.size(); }

int Container::Capacity() { return capacity; }

boost::weak_ptr<Item> Container::GetFirstItem() { return *items.begin(); }

std::set<boost::weak_ptr<Item> >::iterator Container::begin() { return items.begin(); }
std::set<boost::weak_ptr<Item> >::iterator Container::end() { return items.end(); }
bool Container::Full() {
	return (capacity-reservedSpace <= 0);
}

void Container::ReserveSpace(bool res) {
	if (res) ++reservedSpace;
	else --reservedSpace;
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
			if (n < listenersAsUids.size()) listenersAsUids.erase(boost::next(listenersAsUids.begin(), n));
            return;
        }
	}
}

void Container::GetTooltip(int x, int y, Tooltip *tooltip) {
    tooltip->AddEntry(TooltipEntry((boost::format("%s (%d/%d)") % name % size() % (Capacity() + size())).str(), TCODColor::white));
}

void Container::TranslateContainerListeners() {
	listeners.clear();
	for (unsigned int i = 0; i < listenersAsUids.size(); ++i) {
		boost::weak_ptr<Construction> cons = Game::Inst()->GetConstruction(listenersAsUids[i]);
		if (cons.lock() && boost::dynamic_pointer_cast<Stockpile>(cons.lock())) {
			listeners.push_back(boost::dynamic_pointer_cast<Stockpile>(cons.lock()).get());
			LOG("Translated conlistner " << cons.lock()->Name());
		}
	}
}

void Container::AddWater(int amount) {
	if (empty() && filth == 0) water += amount;
}

void Container::RemoveWater(int amount) {
	water -= amount;
	if (water < 0) water = 0;
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