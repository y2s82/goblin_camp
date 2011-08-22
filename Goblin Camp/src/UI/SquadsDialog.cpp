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
#include <cassert>

#include <libtcod.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include "UI/SquadsDialog.hpp"
#include "UI/ScrollPanel.hpp"
#include "UI/Label.hpp"
#include "UI/Button.hpp"
#include "UI/Spinner.hpp"
#include "UI.hpp"
#include "UI/TextBox.hpp"
#include "UI/Frame.hpp"
#include "Announce.hpp"
#include "MapMarker.hpp"

SquadsDialog* SquadsDialog::squadDialog = 0;
SquadsDialog* SquadsDialog::SquadDialog() {
	if (!squadDialog){
		UIContainer *contents = new UIContainer(std::vector<Drawable *>(), 0, 0, 50, 20);
		squadDialog = new SquadsDialog(contents, "Squads", 50, 20);
		squadDialog->squadList = new UIList<std::pair<std::string, boost::shared_ptr<Squad> >, std::map<std::string, boost::shared_ptr<Squad> > >(
			&(Game::Inst()->squadList), 0, 0, 46, 16, SquadsDialog::DrawSquad, boost::bind(&SquadsDialog::SelectSquad, squadDialog, _1), true, &SquadsDialog::GetSquadTooltip);
		Frame *left = new Frame("Existing", std::vector<Drawable *>(), 1, 1, 24, 18);
		left->AddComponent(new ScrollPanel(1, 0, 23, 18, squadDialog->squadList, false));
		contents->AddComponent(left);
		squadDialog->rightFrame = new Frame("New Squad", std::vector<Drawable *>(), 25, 1, 24, 18);
		squadDialog->rightFrame->AddComponent(new Label("Name (required)", 12, 2));
		squadDialog->rightFrame->AddComponent(new TextBox(1, 3, 22, &(squadDialog->squadName)));
		squadDialog->rightFrame->AddComponent(new Label("Members", 12, 5));
		squadDialog->rightFrame->AddComponent(new Spinner(1, 6, 22, &(squadDialog->squadMembers), 1, std::numeric_limits<int>::max()));
		squadDialog->rightFrame->AddComponent(new Label("Priority", 12, 8));
		squadDialog->rightFrame->AddComponent(new Spinner(1, 9, 22, &(squadDialog->squadPriority), 0, std::numeric_limits<int>::max()));
		Button *create = new Button("Create", boost::bind(&SquadsDialog::CreateSquad, squadDialog), 2, 11, 10);
		create->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, false));
		Button *modify = new Button("Modify", boost::bind(&SquadsDialog::ModifySquad, squadDialog), 2, 11, 10);
		modify->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		Button *deleteSquad = new Button("Delete", boost::bind(&SquadsDialog::DeleteSquad, squadDialog), 13, 11, 10);
		deleteSquad->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		squadDialog->rightFrame->AddComponent(create);
		squadDialog->rightFrame->AddComponent(modify);
		squadDialog->rightFrame->AddComponent(deleteSquad);
		contents->AddComponent(squadDialog->rightFrame);
		squadDialog->orders = new Frame("Orders for ", std::vector<Drawable *>(), 0, 20, 50, 5);
		squadDialog->orders->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		contents->AddComponent(squadDialog->orders);
		squadDialog->orders->AddComponent(new ToggleButton("Guard", boost::bind(&SquadsDialog::SelectOrder, squadDialog, GUARD), boost::bind(&SquadsDialog::OrderSelected, squadDialog, GUARD), 2, 1, 9));
		squadDialog->orders->AddComponent(new ToggleButton("Follow", boost::bind(&SquadsDialog::SelectOrder, squadDialog, FOLLOW), boost::bind(&SquadsDialog::OrderSelected, squadDialog, FOLLOW), 14, 1, 10));
		squadDialog->orders->AddComponent(new ToggleButton("Patrol", boost::bind(&SquadsDialog::SelectOrder, squadDialog, PATROL), boost::bind(&SquadsDialog::OrderSelected, squadDialog, PATROL), 27, 1, 10));
		Frame *weapons = new Frame("Weapons", std::vector<Drawable *>(), 0, 25, 23, 5);
		weapons->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		contents->AddComponent(weapons);
		weapons->AddComponent(new LiveButton(boost::bind(&SquadsDialog::SelectedSquadWeapon, squadDialog), boost::bind(&SquadsDialog::SelectWeapon, squadDialog), 1, 1, 21));
		Button *rearm = new Button("Rearm", boost::bind(&SquadsDialog::Rearm, squadDialog), 0, 30, 10);
		rearm->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		contents->AddComponent(rearm);

		Frame *armor = new Frame("Armor", std::vector<Drawable *>(), 23, 25, 23, 5);
		armor->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		contents->AddComponent(armor);
		armor->AddComponent(new LiveButton(boost::bind(&SquadsDialog::SelectedSquadArmor, squadDialog), boost::bind(&SquadsDialog::SelectArmor, squadDialog), 1, 1, 21));
		Button *reequip = new Button("Re-equip", boost::bind(&SquadsDialog::Reequip, squadDialog), 23, 30, 10);
		reequip->SetVisible(boost::bind(&SquadsDialog::SquadSelected, squadDialog, true));
		contents->AddComponent(reequip);
	} 
	return squadDialog;
}

void SquadsDialog::DrawSquad(std::pair<std::string, boost::shared_ptr<Squad> > squadi, int i, int x, int y, int width, bool selected, TCODConsole *console) {
	console->setBackgroundFlag(TCOD_BKGND_SET);
	console->setDefaultBackground(selected ? TCODColor::blue : TCODColor::black);
	console->print(x, y, "%s (%d/%d)", squadi.first.c_str(), squadi.second->MemberCount(),
		squadi.second->MemberLimit());    
	console->setDefaultBackground(TCODColor::black);
}

void SquadsDialog::GetSquadTooltip(std::pair<std::string, boost::shared_ptr<Squad> > squadi, Tooltip *tooltip) {
	tooltip->AddEntry(TooltipEntry(squadi.first, TCODColor::white));
	tooltip->AddEntry(TooltipEntry((boost::format(" Priority: %d") % squadi.second->Priority()).str(), TCODColor::grey));

	if(squadi.second->GetGeneralOrder() != NOORDER) {
		std::string order;
		switch (squadi.second->GetGeneralOrder()) {
		case GUARD:
			order = "Guard";
			break;
		case PATROL:
			order = "Patrol";
			break;
		case FOLLOW:
			order = "Follow";
			break;
		case NOORDER:
			//unreachable as we tested '!= NOORDER'
			assert(false);
		}
		tooltip->AddEntry(TooltipEntry((boost::format(" Orders: %s") % order).str(), TCODColor::grey));
	}
	tooltip->AddEntry(TooltipEntry((boost::format(" Weapon: %s") % Item::ItemCategoryToString(squadi.second->Weapon())).str(), TCODColor::grey));
	tooltip->AddEntry(TooltipEntry((boost::format(" Armor: %s") % Item::ItemCategoryToString(squadi.second->Armor())).str(), TCODColor::grey));
}

boost::shared_ptr<Squad> SquadsDialog::GetSquad(int i) {
	std::map<std::string, boost::shared_ptr<Squad> >::iterator it = Game::Inst()->squadList.begin();
	if (i >= 0 && i < (signed int)Game::Inst()->squadList.size()) {
		return boost::next(it, i)->second;
	}
	return boost::shared_ptr<Squad>();
}

void SquadsDialog::SelectSquad(int i) {
	if(i >= 0 && i < (signed int)Game::Inst()->squadList.size()) {
		rightFrame->SetTitle("Modify Squad");
		squadName = GetSquad(i)->Name();
		squadPriority = GetSquad(i)->Priority();
		squadMembers = GetSquad(i)->MemberLimit();
		orders->SetTitle((boost::format("Orders for %s") % squadName).str());
	} else {
		rightFrame->SetTitle("New Squad");
		squadName = "";
		squadMembers = 1;
		squadPriority = 0;
	}
	RefreshMarkers();
}

bool SquadsDialog::SquadSelected(bool selected) {
	return (squadList->Selected() >= 0) == selected;
}

void SquadsDialog::CreateSquad() {
	if(squadName.length() > 0) {
		Game::Inst()->squadList.insert(std::pair<std::string, boost::shared_ptr<Squad> >
			(squadName, boost::shared_ptr<Squad>(new Squad(squadName, squadMembers, squadPriority))));
		int squad = 0;
		for (std::map<std::string, boost::shared_ptr<Squad> >::iterator it = Game::Inst()->squadList.begin();
			it != Game::Inst()->squadList.end(); ++it) {
				if (it->first == squadName) {
					break;
				}
				++squad;
		}
		squad = std::min(squad, (signed int)Game::Inst()->squadList.size()-1);
		squadList->Select(squad);
		SelectSquad(squad);
	}
}

void SquadsDialog::ModifySquad() {
	boost::shared_ptr<Squad> tempSquad = GetSquad(squadList->Selected());
	Game::Inst()->squadList.erase(tempSquad->Name());
	tempSquad->Name(squadName);
	Game::Inst()->squadList.insert(std::pair<std::string, 
		boost::shared_ptr<Squad> >(squadName, tempSquad));
	tempSquad->MemberLimit(squadMembers);
	tempSquad->Priority(squadPriority);

	//Reselect the squad, changing the name may change it's position in the list
	int squad = 0;
	for (std::map<std::string, boost::shared_ptr<Squad> >::iterator it = Game::Inst()->squadList.begin();
		it != Game::Inst()->squadList.end(); ++it) {
			if (it->first == squadName) {
				break;
			}
			++squad;
	}
	squad = std::min(squad, (signed int)Game::Inst()->squadList.size()-1);
	squadList->Select(squad);
	SelectSquad(squad);

}

void SquadsDialog::DeleteSquad() {
	boost::shared_ptr<Squad> squad = GetSquad(squadList->Selected());
	if (squad) {
		squad->RemoveAllMembers();
		Game::Inst()->squadList.erase(squad->Name());
	}
}

void SquadsDialog::SelectOrder(Order order) {
	boost::shared_ptr<Squad> squad = GetSquad(squadList->Selected());
	if (squad) {
		squad->ClearOrders();
		squad->SetGeneralOrder(order);

		switch (order) {
		case GUARD:
		case PATROL:
		default:
			UI::ChooseOrderTargetCoordinate(squad, order);
			break;

		case FOLLOW:
			UI::ChooseOrderTargetEntity(squad, FOLLOW);
			break;
		}
		UI::Inst()->HideMenu();
	}
}

bool SquadsDialog::OrderSelected(Order order) {
	boost::shared_ptr<Squad> squad = GetSquad(squadList->Selected());
	return squad ? squad->GetGeneralOrder() == order : false;
}

std::string SquadsDialog::SelectedSquadWeapon() {
	int weapon = GetSquad(squadList->Selected())->Weapon();
	return weapon >= 0 ? Item::Categories[weapon].name : "None";
}

void SquadsDialog::SelectWeapon() {
	Menu *weaponChoiceMenu = new Menu(std::vector<MenuChoice>(), "Weapons");
	weaponChoiceMenu->AddChoice(MenuChoice("None", boost::bind(&Squad::Weapon, GetSquad(squadList->Selected()), -1)));
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		if (Item::Categories[i].parent >= 0 && boost::iequals(Item::Categories[Item::Categories[i].parent].name, "Weapon")) {
			weaponChoiceMenu->AddChoice(MenuChoice(Item::Categories[i].name.c_str(), boost::bind(&Squad::Weapon, GetSquad(squadList->Selected()), i)));
		}
	}
	weaponChoiceMenu->ShowModal();
}

void SquadsDialog::Rearm() {
	GetSquad(squadList->Selected())->Rearm();
	Announce::Inst()->AddMsg(GetSquad(squadList->Selected())->Name() + " rearming");
}

std::string SquadsDialog::SelectedSquadArmor() {
	int armor = GetSquad(squadList->Selected())->Armor();
	return armor >= 0 ? Item::Categories[armor].name : "None";
}

void SquadsDialog::SelectArmor() {
	Menu *armorChoiceMenu = new Menu(std::vector<MenuChoice>(), "Armor");
	armorChoiceMenu->AddChoice(MenuChoice("None", boost::bind(&Squad::Armor, GetSquad(squadList->Selected()), -1)));
	for (unsigned int i = 0; i < Item::Categories.size(); ++i) {
		if (Item::Categories[i].parent >= 0 && boost::iequals(Item::Categories[Item::Categories[i].parent].name, "Armor")) {
			armorChoiceMenu->AddChoice(MenuChoice(Item::Categories[i].name.c_str(), boost::bind(&Squad::Armor, GetSquad(squadList->Selected()), i)));
		}
	}
	armorChoiceMenu->ShowModal();
}

void SquadsDialog::Reequip() {
	GetSquad(squadList->Selected())->Reequip();
	Announce::Inst()->AddMsg(GetSquad(squadList->Selected())->Name() + " re-equipping armor");
}

void SquadsDialog::Close() {
	UI::Inst()->SetTextMode(false);
	for (std::list<int>::iterator markeri = markers.begin(); markeri != markers.end();) {
		Map::Inst()->RemoveMarker(*markeri);
		markeri = markers.erase(markeri);
	}
}

void SquadsDialog::RefreshMarkers() {
	for (std::list<int>::iterator markeri = markers.begin(); markeri != markers.end();) {
		Map::Inst()->RemoveMarker(*markeri);
		markeri = markers.erase(markeri);
	}
	boost::shared_ptr<Squad> squad = GetSquad(squadList->Selected());
	if (squad) {
		int orderIndex = 0;
		do { 
			markers.push_back(Map::Inst()->AddMarker(MapMarker(FLASHINGMARKER, 'X', squad->TargetCoordinate(orderIndex), -1, TCODColor::azure)));
			squad->GetOrder(orderIndex);
		} while (orderIndex != 0);
	}
}

void SquadsDialog::Open() {
	RefreshMarkers();
}
