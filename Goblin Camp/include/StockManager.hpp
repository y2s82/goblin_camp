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

#include <map>
#include <set>
#include <list>

#include <boost/weak_ptr.hpp>
#include "data/Serialization.hpp"

class Construction;
typedef int ConstructionType;
typedef int ItemType;
typedef int ItemCategory;
class NatureObject;
class Job;
class Coordinate;

class StockManager {
	GC_SERIALIZABLE_CLASS
	
	StockManager(void);
	static StockManager* instance;
	
	std::map<ItemCategory,int> categoryQuantities;
	std::map<ItemType,int> typeQuantities;
	std::map<ItemType,int> minimums; //If minimum is -1, the product isn't available yet
	std::set<ItemType> producables;
	std::set<ItemType> dumpables;
	std::map<ItemType, ConstructionType> producers;
	std::multimap<ConstructionType, boost::weak_ptr<Construction> > workshops;
	std::set<ItemType> fromTrees; //Trees and stones are a special case, possibly fish in the future as well
	std::set<ItemType> fromEarth;
	std::list<boost::weak_ptr<NatureObject> > designatedTrees;
	std::list<std::pair<boost::weak_ptr<Job>, boost::weak_ptr<NatureObject> > > treeFellingJobs;
	std::set<Coordinate> designatedBog;
	std::list<boost::weak_ptr<Job> > bogIronJobs;
	std::list<boost::weak_ptr<Job> > barrelWaterJobs;
public:
	static StockManager* Inst();
	~StockManager(void);

	void Init();
	static void Reset();
	void Update();
	void UpdateQuantity(ItemType, int);
	int CategoryQuantity(ItemCategory);
	int TypeQuantity(ItemType);
	int Minimum(ItemType);
	void AdjustMinimum(ItemType, int);
	void SetMinimum(ItemType, int);
	std::set<ItemType>* Producables();

	void UpdateWorkshops(boost::weak_ptr<Construction>, bool add);
	void UpdateTreeDesignations(boost::weak_ptr<NatureObject>, bool add);
	void UpdateBogDesignations(Coordinate, bool add);
};

BOOST_CLASS_VERSION(StockManager, 1)
