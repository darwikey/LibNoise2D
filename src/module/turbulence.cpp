// turbulence.cpp
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

#include "module/turbulence.h"

using namespace noise::module;

Turbulence::Turbulence ():
  Module (GetSourceModuleCount ()),
  m_power (DEFAULT_TURBULENCE_POWER)
{
  SetSeed (DEFAULT_TURBULENCE_SEED);
  SetFrequency (DEFAULT_TURBULENCE_FREQUENCY);
  SetRoughness (DEFAULT_TURBULENCE_ROUGHNESS);
}

real Turbulence::GetFrequency () const
{
  // Since each noise::module::Perlin noise module has the same frequency, it
  // does not matter which module we use to retrieve the frequency.
  return m_xDistortModule.GetFrequency ();
}

int Turbulence::GetSeed () const
{
  return m_xDistortModule.GetSeed ();
}

real Turbulence::GetValue (real x, real y) const
{
  assert (m_pSourceModule[0] != NULL);

  // Get the values from the three noise::module::Perlin noise modules and
  // add each value to each coordinate of the input value.  There are also
  // some offsets added to the coordinates of the input values.  This prevents
  // the distortion modules from returning zero if the (x, y, z) coordinates,
  // when multiplied by the frequency, are near an integer boundary.  This is
  // due to a property of gradient coherent noise, which returns zero at
  // integer boundaries.
  real x0, y0;
  real x1, y1;
  real x2, y2;
  x0 = x + (12414.0f / 65536.0f);
  y0 = y + (65124.0f / 65536.0f);
  x1 = x + (26519.0f / 65536.0f);
  y1 = y + (18128.0f / 65536.0f);
  x2 = x + (53820.0f / 65536.0f);
  y2 = y + (11213.0f / 65536.0f);
  real xDistort = x + (m_xDistortModule.GetValue (x0, y0)
    * m_power);
  real yDistort = y + (m_yDistortModule.GetValue (x1, y1)
    * m_power);

  // Retrieve the output value at the offsetted input value instead of the
  // original input value.
  return m_pSourceModule[0]->GetValue (xDistort, yDistort);
}

void Turbulence::SetSeed (int seed)
{
  // Set the seed of each noise::module::Perlin noise modules.  To prevent any
  // sort of weird artifacting, use a slightly different seed for each noise
  // module.
  m_xDistortModule.SetSeed (seed    );
  m_yDistortModule.SetSeed (seed + 1);
  m_zDistortModule.SetSeed (seed + 2);
}
