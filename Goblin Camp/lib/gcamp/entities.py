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
import functools
import _gcampapi

spawn = _gcampapi.spawnEntity
spawn.__doc__ = 'Spawns an entity on the map'

for k in ('Building', 'NPC', 'Plant', 'Item'):
	typeName = 'ENTITY_{0}'.format(k.upper())
	funName  = 'spawn{0}'.format(k)
	
	globals()[typeName] = getattr(_gcampapi.EntityType, typeName)
	globals()[funName]  = functools.partial(spawn, globals()[typeName])
