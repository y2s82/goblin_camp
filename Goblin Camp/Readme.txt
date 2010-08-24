--- Goblin Camp ---

-- About: --
Goblin Camp aims to be a roguelike city builder with an emphasis on
macromanaging the economy and military. I've drawn inspiration from games
such as Anno 1404, Dwarf Fortress and Dungeon Keeper.

Goblin Camp is open source, the full source is available at:
http://bitbucket.org/genericcontainer/goblin-camp
Code contributions are welcome!

-- Version 0.111: ---

Fixed stockmanager not scrolling

-- Version 0.11: --
Lots of small fixes and enhancements, and a few major additions. You can now 
craft weapons to make your orcs more effective fighters, mine stone from 
quarries, establish a spawning pool for more orcs and goblins, and watch bees 
fly over your walls and poison your goblins!

- Short changelog: -
*All data files are in libtcod format now, instead of XML. Modding should be 
	notably easier
*Mods no longer need to modify the .dat files that come with GC, instead you 
	can place them in their own folder under the 'mods' folder, and they'll 
	automatically be read in. (Thanks PiotrLegnica)
*Fixed several bugs (walls display correctly now, for example)
*Creatures have creature specific attacks, and can inflict other effects as 
	well (a bee sting may poison your orcs)
*Stone quarries can be dug out and will produce stone for you. Underground 
	nasties may attack from within, so beware.
*No more random orc/goblin migrants
*You can establish a spawning pool to spawn more recruits
*Constructions and areas can be designated by clicking and dragging, in 
	addition to the original two-click method.
*Flying monsters have been implemented, walls won't keep out everything now!
*Sleeping on the rough ground will have a detrimental effect, so now you have a
	 reason to build beds
*Constructions set to be dismantled display this fact as well
*msvcr100.dll and msvcp100.dll are included in the .zip to make install easier.

New constructions, creatures and items are included, many created by the community. 
A big thanks to everyone who's contributed with mods and/or code!

You can check out the changesets on bitbucket for a more detailed view of what's 
changed and who's done what.

Please report all bugs in the Bug Report forum at:
http://www.goblincamp.com/forum/

-- Basic Controls --

*Right-click to open up the root menu. Clicking outside the menu or 
	right-clicking closes the menu.
*Middle-click moves back one level.
*Spacebar pauses the game
*Press the 'h' key down (and keep it pressed down) to see keyboard help text. 
	Basically there are keyboard shortcuts for most of the menus, and you can use
	the numbers 0-9 to choose a menu item.

- Stock manager controls -

*To quickly find the item you're looking for, type part of its name. 
	The stock manager will automatically sort the items based on your input.
*Either click the +/- buttons to change minimums, or press +/- on your numberpad.
*Holding down shift will change the minimums by 10

-- How to Play --

A good way to start is to place a stockpile by choosing it from the menu 
Constructions -> Basics -> Stockpile. Then left-click the stockpile to open it 
in the sidebar, and shift-click on Wood to allow all wood items in the 
stockpile. Allow the food category as well so that the bread you started out 
with gets stored. If you plan on making crates for planks, allow those too
(plank containers).

Next designate trees for cutting by choosing Orders -> Designate trees, you can
pretty much drag the selection over as big an area as you feel like. The trees 
will only be cut down as needed. Open up the Stock Manager and raise the wood 
log minimum, maybe to 50 or so.

Now that the trees are being cut down, either allow the plant category in the 
same stockpile, or make a separate one.
Designate a farm plot, and allow both seed types there. 
Once spring begins, your goblins should plant all the seeds they have in the 
fields, as long as they are in a stockpile to begin with. For items to be usable
they must be stored in a stockpile.

You'll need planks for various things, so now you should build a carpenter's 
workshop. You can then raise the minimums for planks. Building some wooden clubs
for your military is also a very good idea.

You can create your first military squad by opening the Squads menu. Type in a
name for the squad, set how many orcs you wish to be in it and click Create. 
Note that military orcs will not work in workshops! You can then set them to 
guard a specific spot, escort an npc, and choose which category of weapon they 
should use. If they are wielding nothing and you specify a weapon category, 
they'll automatically pick up weapons. If you later change the category, or 
make better weapons of the same category, click Rearm to order that squad to 
pickup better weapons.

Building walls for your camp is important, wild animals won't be able to attack 
you as freely. Doors will keep out most animals, only creatures with hands are 
able to open them. Flying creatures won't care though, so you have to be wary of 
them at all times.

A note on the Stock Manager. Setting minimums for producable goods like planks 
queues up jobs at the corresponding workshops, but it works slightly differently 
for seeds. A seed minimum actually defines how many seeds will be reserved 
_only_  for planting, and nothing else. You should set this to a value high enough
so that you'll always have seeds for farming and won't accidentally mill them all 
into flour.

You'll want to place your spawning pool to get more orcs and goblins, you'll be
losing them as monsters start harassing you. You only get to place one pool,
and can't move it later, so think before you place it!

-- Configuration --

You can change the resolution and switch between windowed/full screen mode by
editing config.ini which you'll find in your 
/Documents/My Games/Goblin camp/ folder (or equivalent).

Fullscreen is toggled by adding/removing 'fullscreen', like so:

config {
	width = 1920
	height = 1080
	renderer = "SDL"
	fullscreen
}

The above config.ini would set GC to run fullscreen and use 1920x1080
as the resolution.


Licensing information is in COPYING.txt
The libtcod license: LIBTCOD-LICENSE.txt

Copyright 2010 Ilkka Halila
