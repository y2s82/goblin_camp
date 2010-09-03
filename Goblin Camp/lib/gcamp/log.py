# Copyright 2010 Ilkka Halila
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
import logging, sys
import _gcamplog
from . import _getModName

def getLog():
	log = logging.getLogger('gcamp.{0}'.format(_getModName(1)))
	print >>sys.stderr, 'bar'
	
	if not hasattr(log, '_init'):
		handler = logging.StreamHandler(_gcamplog.LoggerStream())
		handler.setFormatter(logging.Formatter(
			'[Mod: {0}] [%(levelname)8s] [%(funcName)s] %(message)s'
		))
		
		log.setLevel(logging.DEBUG)
		log.handlers = [handler]
		log._init = True
	
	return log
