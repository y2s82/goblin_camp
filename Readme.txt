--- Goblin Camp ---

-- About: --
Goblin Camp aims to be a roguelike city builder with an emphasis on
macromanaging the economy and military. I've drawn inspiration from games
such as Anno 1404, Dwarf Fortress and Dungeon Keeper.

Goblin Camp is open source, the full source is available at:
http://bitbucket.org/genericcontainer/goblin-camp
Code contributions are welcome!

-- Version 0.21 --

A bugfix release, mostly with fixes and improvements from contributors: a bunch of fixes from 
xkeitarox, and a rendering patch from quantummy. Also some new content and improvements to the
old stuff from giaotanj.

-- Version 0.2 --

Version 0.2 comes with a pretty major change to stockpiles. They are now called piles, and instead
of having any categories that are (dis)allowed everything gets stored in them. Moving through them is
slow, so it's a good idea to make more than one of them. Containers will get moved from pile to pile
depending on where they are needed the most, so all you need to do is create the containers and they'll
be automatically used.

Also introduced in this version is diseases, which will act as a natural limit to the size of your
settlement unless you do something to counteract the diseases.

*Piles
*Diseases
*Migratory animals
*The spawning pool now expands and spreads corruption in a smooth way instead of just erupting with corruption
*Cowardly creatures may panic if they come in contact with another panicking creature
*Death messages
*Constructions under the mouse cursor strobe to make it easier to figure out where everything is
*Portable mode. Just create a file called 'goblin-camp.portable' in the directory where you have
Goblin Camp in and it'll store everything it needs in that directory instead of using your operating
system's defaults.
*Skeletons no longer bleed



-- Version 0.15 --

v0.15 has a slew of new features, among them seasonal changes (ice!), battlements, new invader ai, 
stat tracking and exotic immigrants. Also old features were balanced and bugs squashed. Version 0.15
also has a rewritten multithreaded pathfinder that should eliminate most of the crashes that were
present in earlier versions.

*Battlements allow your orcs to fire over walls with their bows
*Rain
*Winter - The ground gets covered with snow and the river freezes over
*Terrain overlay, press 't' to view only the terrain + creatures
*Immigrant creatures. Trolls are the only immigrants right now, but more can easily be modded in.
*River width and depth configurable in config.py
*Performance optimizations
*Many tileset renderer improvements both in performance and features 
  (you can find the translucent ui option in the settings menu)
*Statistics are now gathered (production, death, filth generated) and points are calculated from it.
*Items flow downstream
*Rewrite of the pathfinder in terms of threadsafety. Should eliminate most if not all crashes.
*Autosaves
*Keyboard cursor movement was finally fixed. You can use the numberpad instead of the mouse if you want to.
*UI has been improved, scroll bars are better and the menu disappears when you make a choice, though it
  leaves your choice visible as a tooltip.
*Water can be barreled and is now required for several products.
*Creatures avoid fire better.
*Wind now behaves more realistically, and instead of being completely random will only shift a set amount
  from the prevailing direction. This allows better planning against total death by fire.
*The river is now generated slightly smarter, and filling tiles with earth should work more as expected.
*Save compression, saves a lot of space and isn't very notable speed-wise.
*Improved tutorial
*Setting to pause on danger (attack or fire)
*Goblins will now dump surplus items into the spawning pool, handily eliminating the problem of accumulating
  a big pile of wooden clubs from kobold attacks.



-- Version 0.141 --

Version v0.141 fixes (hopefully) all of the crash bugs found in v0.14, as well as the content that was sligthly 
off. Even though this is only a minor version number change the save game format was changed in a major enough 
way that backwards compatibility was broken. This is unfortunate, but the change allows future saves to handle 
.dat file changes gracefully, unlike before, and backwards save compatibility shouldn’t get broken as often.

Also I’ve given many creatures their own factions, so you won’t see bees, giants and wolves living in harmony 
anymore.

Another fairly major change was how mod information is loaded. Now creatures/items/construction/etc can be 
overwritten by mods just by defining an entity with the same name as an existing one.  No more mods that need 
to change core .dats!

*Creatures items are now transferred to the player faction on death, so that you can stockpile the clubs 
kobolds drop for example
*Content fixed
*Dismantling related crashes fixed
*Spawning pool dumping job priority bumped up a notch
*Breastwork is no longer a wall, it is now a construction that simply slows down movement. 
It was dropped down a tier, made cheaper to build, and palisades were bumped up a tier.
*Jobs are cancelled more proactively now by npcs
*Tweaked some messages
*Disallowed digging ditches in rock
*Camp tier can now decrease if your population falls dramatically enough
*Creatures now have more varied factions resulting in more infighting between species
*Mods allowed to overwrite existing data

-- Version 0.14 --

0.14 brings the long awaited tilesupport with it (thanks to Immortius). Also, fire, which adds a whole new
aspect to the game. Lot's of background work too by PiotrLegnica as always.

*Bugfixes, one which was an especially subtle one affecting only some mods on some computers.
*Tilesupport! Long awaited, it's now here. The tileset that comes with v0.14 has been gathered from the forums as
well as crawl-tiles, but is lacking quite a few things, so help is really appreciated if anyone can contribute more
tiles.
*Fire. Imps throw fireballs and orcs can start fires themselves. Fire will burn through the forest pretty quickly
and destroy all things flammable. It's pretty great.
*Wind. Wind direction will affect which way fire will spread, and which way smoke moves
*Basic offensive spells have been added, and can be modded in pretty easily.
*Buckets, which is an important change because goblins can now use them to douse flames if they get too close.
Also dumping filth is done with buckets now instead of barrels.
*Chimneys. Constructions which use fuel to produce their products, such as kitchens, will produce smoke as the orcs
work and really add to the atmosphere. Mind you, working with fire always has its risks...
*Slope is calculated for the entire map depending on the heightmap used to create it, which will make filth
flow downwards. Most of the time this means that filth flows from your camp towards the river, but rocky
outcroppings may cause it go elsewhere.
*Rivers have a current, which can be observed when you have filth flowing into it. The filth will be taken by the
current and move downriver, accumulating on the banks and eventually flowing offmap to be someone else's problem
*The game now tracks how many years have gone by in gametime
*Stockpiles and farmplots can now be dismantled only partially, allowing you to use that and expansion to fine-tune
them to just your liking
*Constructions will be automatically repaired in the event that they get damaged by fighting or fire (constructions
also display their condition in the sidebar now, like creatures)
*Traits have been added. This is mostly just the framework for future things, but for now some goblins are born
less prepared to meet fire, and orcs change appearence depending on if they're fresh from the pool or if they've
already seen some fighting.
*A lot of new content has been added, many items, constructions, creatures and plants
*You can now create poison antidotes, healing tonics and even chillwine to stave weariness off. Creatures will
know to look for drinks and food to fix their ailments by themselves.
*Having all your goblins and orcs die no longer throws you back into the menu straight away, you can opt to watch
the carnage continue if you wish.
*Weapons and tools now take damage as they are used, and will eventually break.
*Creatures can be defined to spawn with equipment, so you can have elves attacked armed with bows and quivers full
of arrows
*The first trap has been added: the spiked pit. It'll help greatly in defence, though your creatures can fall
in too if they step too close, so think about where you place them.


-- Version 0.13 --

0.13 is a big update, lot's of changes. Proper heightmap based map
generation, camp tiers, bridges, tunnelling monsters, a new spawning pool,
and so forth.

*Implemented heightmap based map generation for more interesting maps
*Containers generalized
*Stockpiles have container-specific limits
*Ants can tunnel through walls
*Gameover is registered when all your orcs+goblins are dead
*Assigning orders to a squad now flashes a marker on thr map to better 
visualize it
*Added patrol orders
*Fixed escort orders, and renamed it to 'Follow'
*You can now define what an npc becomes on death (corpse, small corpse, 
just filth, etc.)
*Stockpiles and farmplots can be expanded after being built
*Pontoon bridges
*Health bar for npcs in the sidebar
*NPC's will eat each other if starving (starting with the weakest npc they 
see)
*Ground will get muddy if walked over constantly
*Duckboards can be built over mud to allow faster travel
*Orcs and goblins spread filth
*The spawning pool will grow organically while spreading corruption the more 
filth and corpses you dump into it
*Items have bulk, and heavier items will slow npcs down more than lighter ones 
will
*Reworked npc stats and food numbers (spreadsheets ahoy)
*Added tooltips to menus to explain different constructions
*Added camp tiers: At first only the most basic constructions are available,
and only weak monsters will attack. As you advance through tiers more options 
become available, and tougher monsters appear.
*Tile requirements for constructions, now stone quarries can only be built on 
stone for example.
*If a place is left to itself plants will grow back and peaceful woodland 
creatures will return
*Goblins no longer stockpile things neatly into rows
*Felling trees and digging ditches requires tools
*Added a totem pole which centers the camp
*Added a tutorial to help out newcomers and show how to use python for mods


-- Version 0.12 --

v0.12 brings a lot of internal improvements, improved mod support and
features such as:

*Armor
*More naturally winding river
*Bog for bog iron gathering (work in progress, as it is now gathering
iron is too risk-free)
*More informative tooltips (Stockpiles show what they contain, for ex.)
*Ranged weapons
*Creatures fly if hit hard enough
*Animals with the Anger attribute now get properly angry
*Items get stockpiled immediately when stockpile allowances are changed
*The closest stockpile is chosen when stockpiling items
*Constructions can be renamed
*Settings and keybindings can be changed through an ingame menu
*Creatures know how to swim
*Constructions are damaged by projectiles
*Job assignment now takes distance from the npc to the job into
consideration.


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

--

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
*Keybindings can be changed from the main menu

- Stock manager controls -

*To quickly find the item you're looking for, type part of its name. 
	The stock manager will automatically sort the items based on your input.
*Either click the +/- buttons to change minimums, or press +/- on your numberpad.
*Holding down shift will change the minimums by 10

-- How to Play --

A good way to start is to create a pile by choosing it from the menu 
Constructions -> Basics -> Pile. Goblins will gather up all the items lying
around and store them in the pile.

Next designate trees for cutting by choosing Orders -> Designate trees, you can
pretty much drag the selection over as big an area as you feel like. The trees 
will only be cut down as needed. Open up the Stock Manager and raise the wood 
log minimum, maybe to 50 or so.

Now that the trees are being cut down, it might be a good idea to create another
pile, as only have one will be really inefficient in the long run. Fetching items
from the middle of a pile is slow.

Designate a farm plot, and allow whichever seed types you want there. 
Once spring begins, your goblins should plant all the seeds they have in the 
fields, as long as they are in a pile to begin with. For items to be usable
they must be stored in a pile.

You'll need planks for various things, so now you should build a saw pit. 
You can then raise the minimums for planks. You'll need a carpenter's workshop
for wooden clubs and other wooden items.

You can create your first military squad by opening the Squads menu. Type in a
name for the squad, set how many orcs you wish to be in it and click Create. 
Note that military orcs will not work in workshops! You can then set them to 
guard a specific spot, follow an npc, and choose which category of weapon they 
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

The easiest way to change settings is through the main menu, but you can also
change the settings through config.ini as described below:

You can change the resolution and switch between windowed/full screen mode by
editing config.py which you'll find in your 
/Documents/My Games/Goblin camp/ folder (or equivalent).

--

Licensing information is in COPYING.txt
The libtcod license: LIBTCOD-LICENSE.txt
Goblin Camp crash reporter tool uses SharpZipLib, which is GPL licensed (refer to COPYING.txt).

Copyright 2010-2011 Ilkka Halila

E-Mail: gencontain@gmail.com
Twitter: ihalila
