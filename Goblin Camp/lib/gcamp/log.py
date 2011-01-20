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
import logging, sys
import _gcampapi
from . import utils

def _createLogger(name):
	log = logging.getLogger(name)
	
	if not hasattr(log, '_init'):
		handler = logging.StreamHandler(_gcampapi.LoggerStream())
		handler.setFormatter(logging.Formatter(
			'Python (`%(name)s` @ `%(lineno)d`), `%(funcName)s`, `%(levelname)s`:\n\t'
			'%(message)s'
			'\n================================'
		))
		
		log.setLevel(logging.DEBUG)
		log.handlers = [handler]
		log._init = True
	
	return log

def getLogger():
	'Create and return mod logger'
	
	mod = utils._getModName(2)
	return _createLogger('gcamp.{0}'.format(mod))
