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

#include "UI/ConstructionDialog.hpp"
#include "UI/Button.hpp"
#include "UI/ScrollPanel.hpp"
#include "UI/UIList.hpp"
#include "UI/Label.hpp"
#include "UI/UIComponents.hpp"
#include "UI/Dialog.hpp"
#include "UI/TextBox.hpp"
#include "UI/Spinner.hpp"
#include "UI.hpp"
#include "Game.hpp"

Dialog* ConstructionDialog::constructionInfoDialog = 0;
Construction* ConstructionDialog::cachedConstruct = 0;
Dialog* ConstructionDialog::ConstructionInfoDialog(Construction* cons) {
	if (constructionInfoDialog && cons != cachedConstruct) {
		delete constructionInfoDialog;
		constructionInfoDialog = 0;
	}
	if (!constructionInfoDialog) {
		cachedConstruct = cons;
		ConstructionDialog *dialog = new ConstructionDialog(50, 5);
		constructionInfoDialog = new Dialog(dialog, "", 50, 5);
		if (!cons->HasTag(STOCKPILE) && !cons->HasTag(FARMPLOT)) {
			dialog->AddComponent(new Button("Rename", boost::bind(&ConstructionDialog::Rename, dialog), 12, 1, 10));
			dialog->AddComponent(new Button("Dismantle", boost::bind(&ConstructionDialog::Dismantle, dialog), 28, 1, 13));
		} else {
			dialog->AddComponent(new Button("Rename", boost::bind(&ConstructionDialog::Rename, dialog), 2, 1, 10));
			dialog->AddComponent(new Button("Dismantle", boost::bind(&ConstructionDialog::Dismantle, dialog), 18, 1, 13));
			dialog->AddComponent(new Button("Expand", boost::bind(&ConstructionDialog::Expand, dialog), 37, 1, 10));
		}
		if(cons->HasTag(STOCKPILE)) {
			constructionInfoDialog->SetHeight(40);
			dialog->AddComponent(new Button("All", boost::bind(&Stockpile::SetAllAllowed, static_cast<Stockpile *>(cons), true), 2, 5, 8));
			dialog->AddComponent(new Button("None", boost::bind(&Stockpile::SetAllAllowed, static_cast<Stockpile *>(cons), false), 11, 5, 8));
			dialog->AddComponent(new ScrollPanel(2, 8, 23, 31,
				new UIList<ItemCat>(&Item::Categories, 0, 0, 46, Item::Categories.size(),
				boost::bind(&ConstructionDialog::DrawCategory, cons, _1, _2, _3, _4, _5, _6, _7),
				boost::bind(&Stockpile::SwitchAllowed, static_cast<Stockpile *>(cons), _1, true)), true));
			dialog->AddComponent(new Label("Limits", 30, 7));
			//Construct list of spinners for container limits
			Grid *grid = new Grid(std::vector<Drawable *>(), 1, 0, 0, 48, 46);
			for (int i = 0; i < Item::Categories.size(); ++i) {
				if ((Item::Categories[i].parent >= 0 && boost::iequals(Item::Categories[Item::Categories[i].parent].GetName(), "Container"))) {
					grid->AddComponent(new Label(Item::Categories[i].GetName(), 10, 0));
					grid->AddComponent(new Spinner(0, 0, 20, boost::bind(&Stockpile::GetLimit, static_cast<Stockpile*>(cons), i),
						boost::bind(&Stockpile::AdjustLimit, static_cast<Stockpile*>(cons), i, _1)));
					grid->AddComponent(new Label("       ", 12, 0));
				}
			}

			dialog->AddComponent(new ScrollPanel(24, 8, 23, 31, grid));
		} else if(cons->Producer()) {
			constructionInfoDialog->SetHeight(40);
			dialog->AddComponent(new Label("Job Queue", 2, 5, TCOD_LEFT));
			dialog->AddComponent(new ScrollPanel(2, 6, 23, 34, 
				new UIList<ItemType, std::deque<ItemType> >(cons->JobList(), 0, 0, 20, 34, 
				ConstructionDialog::DrawJob,
				boost::bind(&Construction::CancelJob, cons, _1)),
				false));
			dialog->AddComponent(new Label("Product List", 26, 5, TCOD_LEFT));
			ProductList *productList = new ProductList(cons);
			for (int prodi = 0; prodi < (signed int)cons->Products()->size(); ++prodi) {
				productList->productPlacement.push_back(productList->height);
				productList->height += 2 + Item::Components(cons->Products(prodi)).size();
			}
			dialog->AddComponent(new ScrollPanel(26, 6, 23, 34, 
				productList,
				false));
		}
		constructionInfoDialog->SetTitle(cons->Name());
		dialog->Construct(cons);
	}
	return constructionInfoDialog;
}

void ConstructionDialog::Construct(Construction* cons) { construct = cons; }

void ConstructionDialog::Rename() {
	UIContainer *contents = new UIContainer(std::vector<Drawable *>(), 1, 1, 28, 7);
	contents->AddComponent(new TextBox(0, 1, 28, boost::bind(&Entity::Name, construct), boost::bind(&Entity::Name, construct, _1)));
	contents->AddComponent(new Button("OK", boost::function<void()>(), 11, 3, 6, TCODK_ENTER, true));
	Dialog *renameDialog = new Dialog(contents, "Rename", 30, 8);
	renameDialog->ShowModal();
}

void ConstructionDialog::Dismantle() {
	UI::Inst()->CloseMenu();
	construct->Dismantle();
}

void ConstructionDialog::Expand() {
	boost::function<void(Coordinate, Coordinate)> rectCall = boost::bind(&Stockpile::Expand, static_cast<Stockpile*>(construct), _1, _2);
	boost::function<bool(Coordinate, Coordinate)> placement = boost::bind(Game::CheckPlacement, _1, Coordinate(1,1), 
		Construction::Presets[construct->Type()].tileReqs);
	UI::Inst()->CloseMenu();
	UI::ChooseRectPlacement(rectCall, placement, '=');
}

void ConstructionDialog::DrawCategory(Construction *construct, ItemCat category, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	Stockpile *sp = static_cast<Stockpile*>(construct);
	console->setDefaultForeground(sp->Allowed(i) ? TCODColor::green : TCODColor::red);
	if (category.parent < 0) {
		console->print(x, y, "%c %s", sp->Allowed(i) ? 225 : 224, Item::Categories[i].name.substr(0,width-3).c_str());
	} else {
		if (i+1 < (signed int)Item::Categories.size() && Item::Categories[i+1].parent == category.parent) {
			console->print(x, y, "%c%c %s", 195, sp->Allowed(i) ? 225 : 224, category.name.substr(0,width-4).c_str());
		} else {
			console->print(x, y, "%c%c %s", 192, sp->Allowed(i) ? 225 : 224, category.name.substr(0,width-4).c_str());
		}
	}
	console->setDefaultForeground(TCODColor::white);
}

void ConstructionDialog::DrawJob(ItemType category, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setDefaultForeground(i == 0 ? TCODColor::white : TCODColor::grey);
	console->print(x, y, Item::ItemTypeToString(category).c_str());
	console->setDefaultForeground(TCODColor::white);
}

void ConstructionDialog::ProductList::Draw(int x, int _y, int scroll, int width, int _height, TCODConsole *console) {
	int y = 0;
	for (int prodi = 0; prodi < (signed int)construct->Products()->size() && y < scroll + _height; ++prodi) {
		if (y >= scroll) {
			console->setDefaultForeground(TCODColor::white);
			console->print(x, _y + y - scroll, "%s x%d", Item::ItemTypeToString(construct->Products(prodi)).c_str(), Item::Presets[construct->Products(prodi)].multiplier);
		}
		++y;
		for (int compi = 0; compi < (signed int)Item::Components(construct->Products(prodi)).size() && y < scroll + _height; ++compi) {
			if (y >= scroll) {
				console->setDefaultForeground(TCODColor::white);
				console->putChar(x + 1, _y + y - scroll, compi+1 < (signed int)Item::Components(construct->Products(prodi)).size() ? TCOD_CHAR_TEEE : TCOD_CHAR_SW, TCOD_BKGND_SET);
				console->setDefaultForeground(TCODColor::grey);
				console->print(x + 2, _y + y - scroll, Item::ItemCategoryToString(Item::Components(construct->Products(prodi), compi)).c_str());
			}
			++y;
		}
		++y;
	}
	console->setDefaultForeground(TCODColor::white);
}

int ConstructionDialog::ProductList::TotalHeight() {
	return height;
}

MenuResult ConstructionDialog::ProductList::Update(int x, int y, bool clicked, TCOD_key_t key) {
	for (int i = 0; i < (signed int)productPlacement.size(); ++i) {
		if (y == productPlacement[i]) {
			if(clicked) {
				construct->AddJob(construct->Products(i));
			}
			return MENUHIT;
		}
	}
	return NOMENUHIT;
}
