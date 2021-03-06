// rotatepoint.cpp
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
#include "module/rotatepoint.h"

using namespace noise::module;

RotatePoint::RotatePoint ():
  Module (GetSourceModuleCount ())
{
  SetAngles (DEFAULT_ROTATE_X, DEFAULT_ROTATE_Y, DEFAULT_ROTATE_Z);
}

NOISE_REAL RotatePoint::GetValue (NOISE_REAL x, NOISE_REAL y) const
{
  assert (m_pSourceModule[0] != NULL);

  NOISE_REAL nx = (m_x1Matrix * x) + (m_y1Matrix * y);
  NOISE_REAL ny = (m_x2Matrix * x) + (m_y2Matrix * y);
  NOISE_REAL nz = (m_x3Matrix * x) + (m_y3Matrix * y);
  return m_pSourceModule[0]->GetValue (nx, ny);
}

void RotatePoint::SetAngles (NOISE_REAL xAngle, NOISE_REAL yAngle,
  NOISE_REAL zAngle)
{
  NOISE_REAL xCos, yCos, zCos, xSin, ySin, zSin;
  xCos = cos (xAngle * DEG_TO_RAD);
  yCos = cos (yAngle * DEG_TO_RAD);
  zCos = cos (zAngle * DEG_TO_RAD);
  xSin = sin (xAngle * DEG_TO_RAD);
  ySin = sin (yAngle * DEG_TO_RAD);
  zSin = sin (zAngle * DEG_TO_RAD);

  m_x1Matrix = ySin * xSin * zSin + yCos * zCos;
  m_y1Matrix = xCos * zSin;
  m_z1Matrix = ySin * zCos - yCos * xSin * zSin;
  m_x2Matrix = ySin * xSin * zCos - yCos * zSin;
  m_y2Matrix = xCos * zCos;
  m_z2Matrix = -yCos * xSin * zCos - ySin * zSin;
  m_x3Matrix = -ySin * xCos;
  m_y3Matrix = xSin;
  m_z3Matrix = yCos * xCos;

  m_xAngle = xAngle;
  m_yAngle = yAngle;
  m_zAngle = zAngle;
}
