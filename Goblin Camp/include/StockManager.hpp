#pragma once

#include <map>

#include <boost/weak_ptr.hpp>

#include "Item.hpp"
#include "Construction.hpp"

class StockManager
{
private:
	StockManager(void);
	static StockManager* instance;

	std::map<ItemCategory,int> categoryQuantities;
	std::map<ItemType,int> typeQuantities;
	std::map<ItemType,int> minimums; //If minimun is -1, the product isn't available yet
	std::set<ItemType> producables;
	std::map<ItemType, ConstructionType> producers;
	std::multimap<ConstructionType, boost::weak_ptr<Construction> > workshops;
	std::set<ItemType> fromTrees; //Trees and stones are a special case, possibly fish in the future as well
	std::set<ItemType> fromEarth;
public:
	static StockManager* Inst();
	~StockManager(void);
	void Update();
	void UpdateQuantity(ItemType, int);
	int CategoryQuantity(ItemCategory);
	int TypeQuantity(ItemType);
	int Minimum(ItemType);
	void AdjustMinimum(ItemType, int);
	std::set<ItemType>* Producables();

	void UpdateWorkshops(boost::weak_ptr<Construction>, bool add);
};

