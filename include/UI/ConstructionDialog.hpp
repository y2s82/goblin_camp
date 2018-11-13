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

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include <boost/weak_ptr.hpp>
#include <libtcod.hpp>

#include "UIComponents.hpp"
#include "Construction.hpp"
#include "Dialog.hpp"

class ConstructionDialog : public UIContainer {
private:
	boost::weak_ptr<Construction> construct;
	class ProductList : public Scrollable {
	private:
		boost::weak_ptr<Construction> construct;
	public:
		ProductList(boost::weak_ptr<Construction> nconstruct): construct(nconstruct), height(0), productPlacement(std::vector<int>()) {}
		int height;
		std::vector<int> productPlacement;
		void Draw(int x, int y, int scroll, int width, int height, TCODConsole *);
		int TotalHeight();
		MenuResult Update(int x, int y, bool clicked, TCOD_key_t key);
	};
public:
	ConstructionDialog(int nwidth, int nheight):
	UIContainer(std::vector<Drawable *>(), 0, 0, nwidth, nheight) {}
	static Dialog* constructionInfoDialog;
	static boost::weak_ptr<Construction> cachedConstruct;
	static Dialog* ConstructionInfoDialog(boost::weak_ptr<Construction>);
	void Construct(boost::weak_ptr<Construction>);
	void Rename();
	void Dismantle();
	static void DrawJob(ItemType, int, int, int, int, bool, TCODConsole *);
	void Expand();
	void CancelJob(int job);
};
