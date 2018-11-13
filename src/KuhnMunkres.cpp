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
#include "KuhnMunkres.hpp"

std::vector<int> FindBestMatching(boost::numeric::ublas::matrix<int> costs) {
	int n = costs.size1();
	std::vector<int> lx(n,0), ly(n,0);
	std::vector<int> xy(n,0), yx(n,0);
	std::vector<bool> S(n,false), T(n,false);
	std::vector<int> slack(n,0), slackx(n,0);
	std::vector<int> prev(n,-1);
	for(int x = 0; x < n; x++) {
		for(int y = 0; y < n; y++) {
			lx[x] = std::max(lx[x], costs(x, y));
		}
		ly[x] = 0;
		xy[x] = -1;
		yx[x] = -1;
	}
	
	int numMatched = 0;
	while(numMatched < n) {
		std::vector<int> q(n,0);
		int rd = 0, wr = 0;
		for (int i = 0; i < n; ++i) { S[i] = false; T[i] = false; prev[i] = -1; }
		int root = -1;
		for(int x = 0; x < n; x++) {
			if(xy[x] == -1) {
				root = x;
				q[wr++] = x;
				prev[x] = -2;
				S[x] = true;
				break;
			}
		}
		
		for(int y = 0; y < n; y++) {
			slack[y] = lx[root] + ly[y] - costs(root, y);
			slackx[y] = root;
		}
		
		int px, py;
		bool foundPath = false;
		while(!foundPath) {
			while(rd < wr) {
				int x = q[rd++];
				for(int y = 0; y < n; y++) {
					if(costs(x, y) == lx[x] + ly[y] && !T[y]) {
						if(yx[y] == -1) {
							foundPath = true;
							px = x;
							py = y;
							break;
						}
						T[y] = true;
						q[wr++] = yx[y];
						S[yx[y]] = true;
						prev[yx[y]] = x;
						for(int sy = 0; sy < n; sy++) {
							if(lx[yx[y]] + ly[sy] - costs(yx[y], sy) < slack[sy]) {
								slack[sy] = lx[yx[y]] + ly[sy] - costs(yx[y], sy);
								slackx[sy] = yx[y];
							}
						}
					}
				}
				if(foundPath) {
					break;
				}
			}
			if(foundPath) {
				break;
			}
			int delta = std::numeric_limits<int>::max();
			for(int y = 0; y < n; y++) {
				if(!T[y]) {
					delta = std::min(delta, slack[y]);
				}
			}
			for(int x = 0; x < n; x++) {
				if(S[x]) {
					lx[x] -= delta;
				}
			}
			for(int y = 0; y < n; y++) {
				if(T[y]) {
					ly[y] += delta;
				} else {
					slack[y] -= delta;
				}
			}
			wr = rd = 0;
			for(int y = 0; y < n; y++) {
				if(!T[y] && slack[y] == 0) {
					if(yx[y] == -1) {
						px = slackx[y];
						py = y;
						foundPath = true;
						break;
					} else {
						T[y] = true;
						if(!S[yx[y]]) {
							q[wr++] = yx[y];
							int sx = yx[y];
							int sp = slackx[y];
							S[sx] = true;
							prev[sx] = sp;
							for(int sy = 0;	sy < n; sy++) {
								if(lx[sx] + ly[sy] - costs(sx, sy) < slack[sy]) {
									slack[sy] = lx[sx] + ly[sy] - costs(sx, sy);
									slackx[sy] = sx;
								}
							}
						}
					}
				}
			}
		}
		
		numMatched++;
		for(int cx = px, cy = py, ty; cx != -2; cx = prev[cx], cy = ty) {
			ty = xy[cx];
			yx[cy] = cx;
			xy[cx] = cy;
		}
	}
	
	std::vector<int> rtn;
	for(int x = 0; x < n; x++) {
		rtn.push_back(xy[x]);
	}
	return rtn;
}
