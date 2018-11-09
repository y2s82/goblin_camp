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

#include "UI/AnnounceDialog.hpp"
#include "UI/ScrollPanel.hpp"
#include "Announce.hpp"

void AnnounceDialog::Draw(int x, int y, int scroll, int width, int height, TCODConsole* console) {
	Announce::Inst()->Draw(Coordinate(x + 1, y), scroll, height, console);
}

MenuResult AnnounceDialog::Update(int x, int y, bool clicked, TCOD_key_t key) {
	if(clicked) {
		Announce::Inst()->AnnouncementClicked(y - 1);
	}
	return MENUHIT;
}

int AnnounceDialog::TotalHeight() {
	return Announce::Inst()->AnnounceAmount();
}

Dialog* AnnounceDialog::announcementsDialog = 0;
Dialog* AnnounceDialog::AnnouncementsDialog() {
	if (!announcementsDialog) {
		int width = Game::Inst()->ScreenWidth() - 20;
		int height = Game::Inst()->ScreenHeight() - 20;
		announcementsDialog = new Dialog(
			new ScrollPanel(0, 0, width, height, new AnnounceDialog(), false),
			"Announcements", width, height
		);
	}
	return announcementsDialog;
}
