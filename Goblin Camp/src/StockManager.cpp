#include "StockManager.hpp"
#include "Item.hpp"

#ifdef DEBUG
#include <iostream>
#endif

StockManager* StockManager::instance = 0;
StockManager* StockManager::Inst() {
	if (!instance) {
		instance = new StockManager();
	}
	return instance;
}

StockManager::StockManager(void){
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		quantities.insert(std::pair<ItemCategory,int>(i,0));
	}
}


StockManager::~StockManager(void){
}

void StockManager::Update() {

}

void StockManager::UpdateQuantity(ItemCategory cat, int quantity) {
	quantities[cat] += quantity;
#ifdef DEBUG
	std::cout<<Item::ItemCategoryToString(cat)<<" "<<quantity<<"\n";
#endif
}

int StockManager::Quantity(ItemCategory cat) { return quantities[cat]; }