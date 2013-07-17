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

'''
	This module is responsible for bootstrapping the dev-console namespace.
	It should import useful stuff, create logger, etc.
	
	I tried to include this in the game itself, but I couldn't get it to work.
	Still, it can't be a bad thing to be able to change it without recompiling
	the game.
'''
from __future__ import print_function
import os, time, pprint, sys, functools, __builtin__, cStringIO as StringIO, textwrap
import gcamp

log = gcamp.log._createLogger('<< console >>')

def out(expr):
	'''
		Pretty prints the value of the expression.
	'''
	expr = pprint.pformat(expr)
	
	# TODO: expose console width (cvars expose only absolute width)
	print(textwrap.fill(expr, 80))
