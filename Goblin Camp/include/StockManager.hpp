#pragma once

#include "Item.hpp"

class StockManager
{
private:
	StockManager(void);
	static StockManager* instance;

	std::map<ItemCategory,int> quantities;
public:
	static StockManager* Inst();
	~StockManager(void);
	void Update();
	void UpdateQuantity(ItemCategory, int);
	int Quantity(ItemCategory);
};

