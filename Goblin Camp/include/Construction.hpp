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

#include <vector>
#include <deque>
#include <map>
#include <list>
#include <ticpp.h>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/serialization/serialization.hpp>

#include "Entity.hpp"
#include "Item.hpp"
#include "Container.hpp"

enum BuildResult {
	BUILD_NOMATERIAL = -99999
};

typedef int ConstructionType;

#define NOTFULL (1 << 0)

enum ConstructionTag {
	STOCKPILE,
	FARMPLOT,
	DOOR,
	WALL,
	BED,
	TAGCOUNT
};

struct ConstructionPreset {
    ConstructionPreset();
    int maxCondition;
    std::vector<int> graphic;
    bool walkable;
	std::list<ItemCategory> materials;
	bool producer;
	std::vector<ItemType> products;
	std::string name;
	Coordinate blueprint;
	bool tags[TAGCOUNT];
	Coordinate productionSpot;
	bool dynamic;
};

class Construction : public Entity {
	friend class boost::serialization::access;
	friend class Game;
private:
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const;
	template<class Archive>
	void load(Archive & ar, const unsigned int version);
	BOOST_SERIALIZATION_SPLIT_MEMBER()

protected:
	Construction(ConstructionType = 0, Coordinate = Coordinate(0,0));

	int _condition, maxCondition;
	std::vector<int> graphic;
	TCODColor color;
	ConstructionType _type;
	bool walkable;
	std::list<ItemCategory> materials;
	bool producer;
	std::vector<ItemType> products;
	std::deque<ItemType> jobList;
	int progress;
	void SpawnProductionJob();
	boost::shared_ptr<Container> container;
	boost::shared_ptr<Container> materialsUsed;
	bool stockpile, farmplot;

	void UpdateWallGraphic(bool recurse = true, bool self = true);
public:
	~Construction();

	static Coordinate Blueprint(ConstructionType);
	static Coordinate ProductionSpot(ConstructionType);
	void condition(int);
	int condition();
	virtual void Draw(Coordinate, TCODConsole*);
	int Build();
	ConstructionType type();
	std::list<ItemCategory>* MaterialList();
	bool Producer();
	std::vector<ItemType>* Products();
	ItemType Products(int);
	void AddJob(ItemType);
	virtual void CancelJob(int=0);
	std::deque<ItemType>* JobList();
	ItemType JobList(int);
	virtual int Use();
	static std::vector<ConstructionPreset> Presets;
	static void LoadPresets(ticpp::Document);
	virtual boost::weak_ptr<Container> Storage();
	bool IsStockpile();
	bool IsFarmplot();
	virtual void Update();
};
