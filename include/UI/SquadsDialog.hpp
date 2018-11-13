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

#include <string>
#include <vector>
#include <list>

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>

#include "UIComponents.hpp"
#include "Dialog.hpp"
#include "UIList.hpp"
#include "Frame.hpp"

class SquadsDialog : public Dialog {
private:
	std::string squadName;
	int squadMembers;
	int squadPriority;
	boost::shared_ptr<Squad> GetSquad(int);
	UIList<std::pair<std::string, boost::shared_ptr<Squad> >, std::map<std::string, boost::shared_ptr<Squad> > > *squadList;
	Frame *rightFrame;
	Frame *orders;
	std::list<int> markers;
	void RefreshMarkers();
public:
	SquadsDialog(Drawable *ncontents, std::string ntitle, int nwidth, int nheight):
		Dialog(ncontents, ntitle, nwidth, nheight), squadName(""), squadMembers(1), squadPriority(0) {}
	static SquadsDialog* squadDialog;
	static SquadsDialog* SquadDialog();
	static void DrawSquad(std::pair<std::string, boost::shared_ptr<Squad> >, int, int, int, int, bool, TCODConsole *);
	static void GetSquadTooltip(std::pair<std::string, boost::shared_ptr<Squad> >, Tooltip *);
	void SelectSquad(int i);
	bool SquadSelected(bool selected);
	void CreateSquad();
	void ModifySquad();
	void DeleteSquad();
	void SelectOrder(Order order);
	bool OrderSelected(Order order);
	std::string SelectedSquadWeapon();
	void SelectWeapon();
	void Rearm();
	std::string SelectedSquadArmor();
	void SelectArmor();
	void Reequip();
	virtual void Close();
	virtual void Open();
};
