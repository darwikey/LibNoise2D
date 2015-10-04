// voronoi.cpp
//
// Copyright (C) 2003, 2004 Jason Bevins
//
// This library is free software; you can redistribute it and/or modify it
// under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation; either version 2.1 of the License, or (at
// your option) any later version.
//
// This library is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
// License (COPYING.txt) for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this library; if not, write to the Free Software Foundation,
// Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// The developer's email is jlbezigvins@gmzigail.com (for great email, take
// off every 'zig'.)
//

#include "mathconsts.h"
#include "module/voronoi.h"

using namespace noise::module;

Voronoi::Voronoi() :
	Module(GetSourceModuleCount()),
	m_displacement(DEFAULT_VORONOI_DISPLACEMENT),
	m_enableDistance(false),
	m_frequency(DEFAULT_VORONOI_FREQUENCY),
	m_seed(DEFAULT_VORONOI_SEED)
{
}

NOISE_REAL Voronoi::GetValue(NOISE_REAL x, NOISE_REAL y) const
{
	// This method could be more efficient by caching the seed values.  Fix
	// later.

	x *= m_frequency;
	y *= m_frequency;

	int xInt = (x > 0.0 ? (int)x : (int)x - 1);
	int yInt = (y > 0.0 ? (int)y : (int)y - 1);

	NOISE_REAL minDist = 2147483647.0f;
	NOISE_REAL xCandidate = 0;
	NOISE_REAL yCandidate = 0;

	// Inside each unit cube, there is a seed point at a random position.  Go
	// through each of the nearby cubes until we find a cube with a seed point
	// that is closest to the specified position.
	for (int yCur = yInt - 2; yCur <= yInt + 2; yCur++) {
		for (int xCur = xInt - 2; xCur <= xInt + 2; xCur++) {

			// Calculate the position and distance to the seed point inside of
			// this unit cube.
			NOISE_REAL xPos = xCur + ValueNoise2D(xCur, yCur, m_seed);
			NOISE_REAL yPos = yCur + ValueNoise2D(xCur, yCur, m_seed + 1);
			NOISE_REAL xDist = xPos - x;
			NOISE_REAL yDist = yPos - y;
			NOISE_REAL dist = xDist * xDist + yDist * yDist;

			if (dist < minDist) {
				// This seed point is closer to any others found so far, so record
				// this seed point.
				minDist = dist;
				xCandidate = xPos;
				yCandidate = yPos;
			}
		}
	}


	NOISE_REAL value;
	if (m_enableDistance) {
		// Determine the distance to the nearest seed point.
		NOISE_REAL xDist = xCandidate - x;
		NOISE_REAL yDist = yCandidate - y;
		value = (sqrt(xDist * xDist + yDist * yDist)
			) * SQRT_3 - 1.0f;
	}
	else {
		value = 0.0;
	}

	// Return the calculated distance with the displacement value applied.
	return value + (m_displacement * (NOISE_REAL)ValueNoise2D(
		(int)(floor(xCandidate)),
		(int)(floor(yCandidate))));
}
