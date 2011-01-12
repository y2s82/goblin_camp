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
import _gcampconfig

setCVar = _gcampconfig.setCVar
setCVar.__doc__ = 'Changes configuration variable (be aware that all changes will be saved into permanent configuration).'
getCVar = _gcampconfig.getCVar
getCVar.__doc__ = 'Retrieves value of a configuration variable.'

bindKey = _gcampconfig.bindKey
bindKey.__doc__ = 'Binds a keycode to a named key (be aware that all changes will be saved into permanent configuration).'
getKey = _gcampconfig.getKey
getKey.__doc__ = 'Retrieves a keycode bound to a named key.'
