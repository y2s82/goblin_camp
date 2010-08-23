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

#include "UI/YesNoDialog.hpp"
#include "UI/Dialog.hpp"
#include "UI/Label.hpp"
#include "UI/Button.hpp"

void YesNoDialog::ShowYesNoDialog(std::string text, boost::function<void()> leftAction, boost::function<void()> rightAction, std::string leftButton, std::string rightButton) {
    UIContainer *contents = new UIContainer(std::vector<Drawable *>(), 0, 0, 50, 10);
    Dialog *dialog = new Dialog(contents, "", 50, 10);
    contents->AddComponent(new Label(text, 25, 2));
    contents->AddComponent(new Button(leftButton, leftAction, 10, 4, 10, 'y'));
    contents->AddComponent(new Button(rightButton, rightAction, 30, 4, 10, 'n'));
    dialog->ShowModal();
}

