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

#include <boost/weak_ptr.hpp>
#include <set>

#include "Item.hpp"

class Container : public Item {
    private:
        std::set<boost::weak_ptr<Item> > items;
        int capacity;
        int reservedSpace;
	public:
        Container(Coordinate, int type, int cap=1000, int faction = 0);
		virtual ~Container();
		virtual bool AddItem(boost::weak_ptr<Item>);
		virtual void RemoveItem(boost::weak_ptr<Item>);
		void ReserveSpace(bool);
		boost::weak_ptr<Item> GetItem(boost::weak_ptr<Item>);
		std::set<boost::weak_ptr<Item> >* GetItems();
		boost::weak_ptr<Item> GetFirstItem();
        bool empty();
        int size();
        int Capacity();
        bool Full();
        std::set<boost::weak_ptr<Item> >::iterator begin();
        std::set<boost::weak_ptr<Item> >::iterator end();
};

