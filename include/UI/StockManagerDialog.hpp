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

#include <libtcod.hpp>

#include "UIComponents.hpp"
#include "Dialog.hpp"
#include "Grid.hpp"

class StockManagerDialog : public Dialog {
private:
	std::string filter;
	Grid *grid;
public:
	StockManagerDialog();
	static Dialog* stocksDialog;
	static Dialog* StocksDialog();
	std::string GetFilter();
};
