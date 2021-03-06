// interp.h
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

#ifndef NOISE_INTERP_H
#define NOISE_INTERP_H

#include "mathconsts.h"

namespace noise
{

  /// @addtogroup libnoise
  /// @{

  /// Performs cubic interpolation between two values bound between two other
  /// values.
  ///
  /// @param n0 The value before the first value.
  /// @param n1 The first value.
  /// @param n2 The second value.
  /// @param n3 The value after the second value.
  /// @param a The alpha value.
  ///
  /// @returns The interpolated value.
  ///
  /// The alpha value should range from 0.0 to 1.0.  If the alpha value is
  /// 0.0, this function returns @a n1.  If the alpha value is 1.0, this
  /// function returns @a n2.
  inline NOISE_REAL CubicInterp (NOISE_REAL n0, NOISE_REAL n1, NOISE_REAL n2, NOISE_REAL n3,
    NOISE_REAL a)
  {
	  NOISE_REAL p = (n3 - n2) - (n0 - n1);
	  NOISE_REAL q = (n0 - n1) - p;
	  NOISE_REAL r = n2 - n0;
	  NOISE_REAL s = n1;
	  return p * a * a * a + q * a * a + r * a + s;
  }

  /// Performs linear interpolation between two values.
  ///
  /// @param n0 The first value.
  /// @param n1 The second value.
  /// @param a The alpha value.
  ///
  /// @returns The interpolated value.
  ///
  /// The alpha value should range from 0.0 to 1.0.  If the alpha value is
  /// 0.0, this function returns @a n0.  If the alpha value is 1.0, this
  /// function returns @a n1.
  inline NOISE_REAL LinearInterp (NOISE_REAL n0, NOISE_REAL n1, NOISE_REAL a)
  {
    return ((1.0f - a) * n0) + (a * n1);
  }

  /// Maps a value onto a cubic S-curve.
  ///
  /// @param a The value to map onto a cubic S-curve.
  ///
  /// @returns The mapped value.
  ///
  /// @a a should range from 0.0 to 1.0.
  ///
  /// The derivitive of a cubic S-curve is zero at @a a = 0.0 and @a a =
  /// 1.0
  inline NOISE_REAL SCurve3 (NOISE_REAL a)
  {
    return (a * a * (3.0f - 2.0f * a));
  }

  /// Maps a value onto a quintic S-curve.
  ///
  /// @param a The value to map onto a quintic S-curve.
  ///
  /// @returns The mapped value.
  ///
  /// @a a should range from 0.0 to 1.0.
  ///
  /// The first derivitive of a quintic S-curve is zero at @a a = 0.0 and
  /// @a a = 1.0
  ///
  /// The second derivitive of a quintic S-curve is zero at @a a = 0.0 and
  /// @a a = 1.0
  inline NOISE_REAL SCurve5 (NOISE_REAL a)
  {
    NOISE_REAL a3 = a * a * a;
    NOISE_REAL a4 = a3 * a;
    NOISE_REAL a5 = a4 * a;
    return (6.0f * a5) - (15.0f * a4) + (10.0f * a3);
  }

  // @}

}

#endif
