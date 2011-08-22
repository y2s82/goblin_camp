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

#include <boost/algorithm/string.hpp>

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
#include "Stockpile.hpp"

Dialog* ConstructionDialog::constructionInfoDialog = 0;
boost::weak_ptr<Construction> ConstructionDialog::cachedConstruct = boost::weak_ptr<Construction>();
Dialog* ConstructionDialog::ConstructionInfoDialog(boost::weak_ptr<Construction> wcons) {
	if (boost::shared_ptr<Construction> cons = wcons.lock()) {
		if (constructionInfoDialog && (!cachedConstruct.lock() || cons != cachedConstruct.lock())) {
			delete constructionInfoDialog;
			constructionInfoDialog = 0;
		}
		if (!constructionInfoDialog) {
			cachedConstruct = cons;
			ConstructionDialog *dialog = new ConstructionDialog(50, 5);
			constructionInfoDialog = new Dialog(dialog, "", 50, 5);
			if (!cons->HasTag(FARMPLOT)) {
				dialog->AddComponent(new Button("Rename", boost::bind(&ConstructionDialog::Rename, dialog), 12, 1, 10));
				dialog->AddComponent(new Button("Dismantle", boost::bind(&ConstructionDialog::Dismantle, dialog), 28, 1, 13));
			} else {
				dialog->AddComponent(new Button("Rename", boost::bind(&ConstructionDialog::Rename, dialog), 2, 1, 10));
				dialog->AddComponent(new Button("Dismantle", boost::bind(&ConstructionDialog::Dismantle, dialog), 18, 1, 13));
				dialog->AddComponent(new Button("Expand", boost::bind(&ConstructionDialog::Expand, dialog), 37, 1, 10));
			}

			if (cons->Producer()) {
				constructionInfoDialog->SetHeight(40);
				dialog->AddComponent(new Label("Job Queue", 2, 5, TCOD_LEFT));
				dialog->AddComponent(new ScrollPanel(2, 6, 23, 34, 
					new UIList<ItemType, std::deque<ItemType> >(cons->JobList(), 0, 0, 20, 34, 
					ConstructionDialog::DrawJob,
					boost::bind(&ConstructionDialog::CancelJob, dialog, _1)),
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
	} else if (constructionInfoDialog) {
		delete constructionInfoDialog;
		constructionInfoDialog = 0;
	}
	return constructionInfoDialog;
}

void ConstructionDialog::Construct(boost::weak_ptr<Construction> cons) { construct = cons; }

void ConstructionDialog::Rename() {
	if (construct.lock()) {
		UIContainer *contents = new UIContainer(std::vector<Drawable *>(), 1, 1, 28, 7);
		contents->AddComponent(new TextBox(0, 1, 28, boost::bind(&Entity::Name, construct.lock()), boost::bind(&Entity::Name, construct.lock(), _1)));
		contents->AddComponent(new Button("OK", boost::function<void()>(), 11, 3, 6, TCODK_ENTER, true));
		Dialog *renameDialog = new Dialog(contents, "Rename", 30, 8);
		renameDialog->ShowModal();
	}
}

void ConstructionDialog::Dismantle() {
	if (construct.lock()) {
		UI::Inst()->CloseMenu();
		construct.lock()->Dismantle(undefined);
	}
}

void ConstructionDialog::Expand() {
	if (construct.lock()) {
		boost::function<void(Coordinate, Coordinate)> rectCall = boost::bind(&Stockpile::Expand, boost::static_pointer_cast<Stockpile>(construct.lock()), _1, _2);
		boost::function<bool(Coordinate, Coordinate)> placement = boost::bind(Game::CheckPlacement, _1, Coordinate(1,1), 
			Construction::Presets[construct.lock()->Type()].tileReqs);
		UI::Inst()->CloseMenu();
		UI::ChooseRectPlacementCursor(rectCall, placement, Cursor_Stockpile);
	}
}

void ConstructionDialog::CancelJob(int job) {
	if (boost::shared_ptr<Construction> cons = construct.lock()) {
		cons->CancelJob(job);
	}
}

void ConstructionDialog::DrawJob(ItemType category, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setDefaultForeground(i == 0 ? TCODColor::white : TCODColor::grey);
	console->print(x, y, Item::ItemTypeToString(category).c_str());
	console->setDefaultForeground(TCODColor::white);
}

void ConstructionDialog::ProductList::Draw(int x, int _y, int scroll, int width, int _height, TCODConsole *console) {
	if (boost::shared_ptr<Construction> cons = construct.lock()) {
		int y = 0;
		for (int prodi = 0; prodi < (signed int)cons->Products()->size() && y < scroll + _height; ++prodi) {
			if (y >= scroll) {
				console->setDefaultForeground(TCODColor::white);
				console->print(x, _y + y - scroll, "%s x%d", Item::ItemTypeToString(cons->Products(prodi)).c_str(), Item::Presets[cons->Products(prodi)].multiplier);
			}
			++y;
			for (int compi = 0; compi < (signed int)Item::Components(cons->Products(prodi)).size() && y < scroll + _height; ++compi) {
				if (y >= scroll) {
					console->setDefaultForeground(TCODColor::white);
					console->putChar(x + 1, _y + y - scroll, compi+1 < (signed int)Item::Components(cons->Products(prodi)).size() ? TCOD_CHAR_TEEE : TCOD_CHAR_SW, TCOD_BKGND_SET);
					console->setDefaultForeground(TCODColor::grey);
					console->print(x + 2, _y + y - scroll, Item::ItemCategoryToString(Item::Components(cons->Products(prodi), compi)).c_str());
				}
				++y;
			}
			++y;
		}
	}
	console->setDefaultForeground(TCODColor::white);
}

int ConstructionDialog::ProductList::TotalHeight() {
	return height;
}

MenuResult ConstructionDialog::ProductList::Update(int x, int y, bool clicked, TCOD_key_t key) {
	for (int i = 0; i < (signed int)productPlacement.size(); ++i) {
		if (y == productPlacement[i]) {
			if(clicked && construct.lock()) {
				construct.lock()->AddJob(construct.lock()->Products(i));
			}
			return MENUHIT;
		}
	}
	return NOMENUHIT;
}
