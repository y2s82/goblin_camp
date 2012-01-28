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
#include "UI/Label.hpp"
#include "UI/TextBox.hpp"

Dialog* StockManagerDialog::stocksDialog = 0;

class StockPanel: public UIContainer {
private:
	ItemType itemType;
	StockManagerDialog *owner;
public:
	bool ShowItem() {
		if (boost::icontains(Item::Presets[itemType].name, owner->GetFilter()))
			return StockManager::Inst()->TypeQuantity(itemType) > -1;
		else {
			for (std::set<ItemCategory>::iterator cati = Item::Presets[itemType].categories.begin();
				cati != Item::Presets[itemType].categories.end(); ++cati) {
					if (boost::icontains(Item::Categories[*cati].name, owner->GetFilter()))
						return StockManager::Inst()->TypeQuantity(itemType) > -1;
			}
		}
		return false;
	}
	
	void _GetTooltip(int x, int y, Tooltip *tooltip) {
		if(x >= _x && x < _x + width && y >= _y && y < _y + height - 2) { // subtract 2 from height so tooltip doesn't appear when mouse is over spinner
			tooltip->AddEntry(TooltipEntry(Item::ItemTypeToString(itemType), TCODColor::white));
			std::string compName = "";
			int compAmt = 0;
			for (int compi = 0; compi < (signed int)Item::Components(itemType).size(); ++compi) {
				std::string thisCompName = Item::ItemCategoryToString(Item::Components(itemType, compi));
				if(compName == thisCompName) {
					compAmt++;
				} else {
					if(compName.length() > 0) {
						tooltip->AddEntry(TooltipEntry((boost::format(" %s x%d") % compName % compAmt).str(), TCODColor::grey));
					}
					compName = thisCompName;
					compAmt = 1;
				}
			}
			if(compName.length() > 0) {
				tooltip->AddEntry(TooltipEntry((boost::format(" %s x%d") % compName % compAmt).str(), TCODColor::grey));
			}
		}
	}
	
	StockPanel(ItemType nItemType, StockManagerDialog *nowner): UIContainer(std::vector<Drawable *>(), 0, 0, 16, 4), itemType(nItemType), owner(nowner) {
		AddComponent(new Spinner(0, 2, 16, boost::bind(&StockManager::Minimum, StockManager::Inst(), itemType), 
								 boost::bind(&StockManager::SetMinimum, StockManager::Inst(), itemType, _1)));
		SetTooltip(boost::bind(&StockPanel::_GetTooltip, this, _1, _2, _3));
		visible = boost::bind(&StockPanel::ShowItem, this);
	}
	
	void Draw(int x, int y, TCODConsole *console) {
		console->setAlignment(TCOD_CENTER);
		console->setDefaultForeground(Item::Presets[itemType].color);
		console->print(x + 8, y, "%c %s", Item::Presets[itemType].graphic, Item::Presets[itemType].name.c_str());
		console->setDefaultForeground(TCODColor::white);
		console->print(x + 8, y+1, "%d", StockManager::Inst()->TypeQuantity(itemType));
		UIContainer::Draw(x, y, console);
	}

};


StockManagerDialog::StockManagerDialog(): Dialog(0, "Stock Manager", 68, 75), filter("")
{
	contents = new UIContainer(std::vector<Drawable *>(), 0, 0, 68, 75);

	static_cast<UIContainer*>(contents)->AddComponent(new Label("Filter", 34, 1));
	static_cast<UIContainer*>(contents)->AddComponent(new TextBox(1, 2, 66, &filter));

	Grid *grid = new Grid(std::vector<Drawable *>(), 4, 0, 0, 68, 71);
	for (std::set<ItemType>::iterator it = StockManager::Inst()->Producables()->begin(); it != StockManager::Inst()->Producables()->end(); it++) {
		grid->AddComponent(new StockPanel(*it, this));
	}
	static_cast<UIContainer*>(contents)->AddComponent(new ScrollPanel(0, 3, 68, 72, grid, false, 4));
}

Dialog* StockManagerDialog::StocksDialog() {
	if (!stocksDialog) stocksDialog = new StockManagerDialog();
	return stocksDialog;
}

std::string StockManagerDialog::GetFilter() { return filter; }
