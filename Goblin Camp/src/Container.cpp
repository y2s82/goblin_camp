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

#include "Container.hpp"
#include "Logger.hpp"

Container::Container(Coordinate pos, ItemType type, int capValue, int faction) : Item(pos, type, faction),
	capacity(capValue),
	reservedSpace(0)
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
		return true;
	}
	return false;
}

void Container::RemoveItem(boost::weak_ptr<Item> item) {
	items.erase(item);
	++capacity;
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
