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

#include <string>

#include <libtcod.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "UI/StockManagerDialog.hpp"
#include "UI/ScrollPanel.hpp"
#include "StockManager.hpp"
#include "UI/Spinner.hpp"

Dialog* StockManagerDialog::stocksDialog = 0;

class StockPanel: public UIContainer {
private:
    ItemType itemType;
    StockManagerDialog *owner;
public:
    bool ShowItem() {
        // TODO filter
        return StockManager::Inst()->TypeQuantity(itemType) > -1;
    }    
    
    StockPanel(ItemType nItemType, StockManagerDialog *nowner): UIContainer(std::vector<Drawable *>(), 0, 0, 16, 4), itemType(nItemType), owner(nowner) {
        AddComponent(new Spinner(0, 2, 16, boost::bind(&StockManager::Minimum, StockManager::Inst(), itemType), 
                                 boost::bind(&StockManager::SetMinimum, StockManager::Inst(), itemType, _1)));
        visible = boost::bind(&StockPanel::ShowItem, this);
    }
    
    void Draw(int x, int y, TCODConsole *console) {
        console->setAlignment(TCOD_CENTER);
        console->setForegroundColor(Item::Presets[itemType].color);
        console->print(x + 8, y, "%c %s", Item::Presets[itemType].graphic, Item::Presets[itemType].name.c_str());
        console->setForegroundColor(TCODColor::white);
        console->print(x + 8, y+1, "%d", StockManager::Inst()->TypeQuantity(itemType));
        UIContainer::Draw(x, y, console);
    }
    
};


StockManagerDialog::StockManagerDialog(): Dialog(0, "Stock Manager", 50, 50), filter("")
{
    Grid *grid = new Grid(std::vector<Drawable *>(), 3, 0, 0, 48, 48);
	for (std::set<ItemType>::iterator it = StockManager::Inst()->Producables()->begin(); it != StockManager::Inst()->Producables()->end(); it++) {
        grid->AddComponent(new StockPanel(*it, this));
    }
    contents = new ScrollPanel(0, 1, 50, 50, grid, false, 4);
}

Dialog* StockManagerDialog::StocksDialog() {
	if (!stocksDialog) stocksDialog = new StockManagerDialog();
	return stocksDialog;
}
