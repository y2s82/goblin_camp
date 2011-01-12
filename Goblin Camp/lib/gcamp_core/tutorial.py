# Copyright 2010-2011 Ilkka Halila
# This file is part of Goblin Camp.
# 
# Goblin Camp is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Goblin Camp is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License 
# along with Goblin Camp. If not, see <http://www.gnu.org/licenses/>.
#

import gcamp
import gcamp.config
import gcamp.events

class Tutorial (gcamp.events.EventListener):

	def onGameStart(self):
		if gcamp.config.getCVar("tutorial") == '1':
			self.stage = 0
		else:
			self.stage = -1
		
		if self.stage == 0:
			gcamp.config.setCVar("tutorial", '0')
			gcamp.delay(100, lambda : gcamp.ui.messageBox("Welcome to the Goblin Camp tutorial!              "+
						"You're best off by first building a stockpile:    " +
						"                                                  " +
						"        -Right-click to open the menu             " +
						"        -choose Build->Basics->Stockpile          " +
						"                                                  " +
						"Then just either click once for each corner of    " +
						"the area, or click and drag the area.             "))

	def onBuildingCreated(self, construct, x, y):
		if self.stage == 0 and construct.getTypeString().lower() == "stockpile":
			gcamp.ui.messageBox("Good work! Now left-click on the stockpile, and   " +
						"click on the 'All' button to allow all item       " +
						"categories. Your goblins should get to work       " +
						"storing everything in there.")

			gcamp.delay(400, lambda : gcamp.ui.messageBox( "Next you should do two things:                    " +
						"                                                  " +
						"    -Choose Orders->Designate trees and designate " +
						"trees that can be chopped down for wood.          " +
						"    -Open the Stockmanager and shift+click on the " +
						"+-sign underneath 'Wood log' a few times.         " +
						"                                                  " +
						"Soon you should see two of your goblins grab the  " +
						"two axes and go forth to chop some trees down.    "))
			self.stage = 10
		elif self.stage == 20 and construct.getTypeString().lower() == "saw pit":
			gcamp.ui.messageBox("Now order up some planks from the                 " +
						"Stock Manager, we'll need those for more advanced " + 
						"constructions.")
			self.stage = 30
			
	def onItemCreated(self, item, x, y):
		if self.stage == 10 and item.getTypeString().lower() == "wood log":
			gcamp.delay(75, lambda : gcamp.ui.messageBox( "Alright, now you have wood logs! Once the logs    " +
						"have been stored in the stockpile, choose         " +
						"Build->Workshops->Saw pit.                        " +
						"Place it somewhere convenient"))
			gcamp.delay(450, lambda : gcamp.ui.messageBox("Now would be a good time to designate a farmplot. " +
						"Choose some suitable place, designate it, and     " +
						"enable planting of all three seed types by        " +
						"left-clicking the farmplot and clicking the boxes " +
						"in the sidebar."))
			self.stage = 20
			
		elif self.stage == 30 and item.getTypeString().lower() == "wood plank":
			gcamp.delay(75, lambda : gcamp.ui.messageBox( "Now build a carpenter, it'll allow you to build   " +
						"crates to store most items and wooden clubs for   " +
						"your future military"))
			gcamp.delay(250, lambda : gcamp.ui.messageBox("It's a good idea to check out what your territory " +
						"looks like time to time.                          " +
						"        Territory->Toggle territory overlay       " +
						"It'll expand automatically as you build things,   " +
						"you just need to remember that goblins will mostly" +
						"_not_ venture outside your territory to pick items" +
						"up or otherwise. They will favour a drinking spot " +
						"inside your territory as well"))
			gcamp.delay(450, lambda : gcamp.ui.messageBox("This is the end of the tutorial.                  " +
						"It's a good idea to choose a site for the spawning" +
						"pool soon. Most constructions will have a         " +
						"description in their tooltip, so look through the " +
						"menus for what you can build. As you build more,  " +
						"and get more orcs and goblins from the spawning   " +
						"pool you'll advance through tiers and gain access " +
						"to more advanced buildings.                       " +
						"Also don't forget about defenses. Build a palisade" +
						"and establish some guard squads!"))

			self.stage = 40

gcamp.events.register(Tutorial())
