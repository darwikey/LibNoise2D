// line.h
//
// Copyright (C) 2004 Keith Davies
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

#ifndef NOISE_MODEL_LINE_H
#define NOISE_MODEL_LINE_H

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include "../module/modulebase.h"

namespace noise
{

  namespace model
  {

    /// @addtogroup libnoise
    /// @{

    /// @addtogroup models
    /// @{

    /// Model that defines the displacement of a line segment.
    ///
    /// This model returns an output value from a noise module given the
    /// one-dimensional coordinate of an input value located on a line
    /// segment, which can be used as displacements.
    ///
    /// This class is useful for creating:
    ///  - roads and rivers
    ///  - disaffected college students
    ///
    /// To generate an output value, pass an input value between 0.0 and 1.0
    /// to the GetValue() method.  0.0 represents the start position of the
    /// line segment and 1.0 represents the end position of the line segment.
    class Line
    {

      public:

        /// Constructor.
        Line ();

        /// Constructor
        ///
        /// @param module The noise module that is used to generate the output
        /// values.
        Line (const module::Module& module);

        /// Returns a flag indicating whether the output value is to be
        /// attenuated (moved toward 0.0) as the ends of the line segment are
        /// approached by the input value.
        ///
        /// @returns
        /// - @a true if the value is to be attenuated
        /// - @a false if not.
        bool GetAttenuate () const
        {
          return m_attenuate;
        }

        /// Returns the noise module that is used to generate the output
        /// values.
        ///
        /// @returns A reference to the noise module.
        ///
        /// @pre A noise module was passed to the SetModule() method.
        const module::Module& GetModule () const
        {
          assert (m_pModule != NULL);
          return *m_pModule;
        }

        /// Returns the output value from the noise module given the
        /// one-dimensional coordinate of the specified input value located
        /// on the line segment.
        ///
        /// @param p The distance along the line segment (ranges from 0.0
        /// to 1.0)
        ///
        /// @returns The output value from the noise module.
        ///
        /// @pre A noise module was passed to the SetModule() method.
        /// @pre The start and end points of the line segment were specified.
        ///
        /// The output value is generated by the noise module passed to the
        /// SetModule() method.  This value may be attenuated (moved toward
        /// 0.0) as @a p approaches either end of the line segment; this is
        /// the default behavior.
        ///
        /// If the value is not to be attenuated, @a p can safely range
        /// outside the 0.0 to 1.0 range; the output value will be
        /// extrapolated along the line that this segment is part of.
        real GetValue (real p) const;

        /// Sets a flag indicating that the output value is to be attenuated
        /// (moved toward 0.0) as the ends of the line segment are approached.
        ///
        /// @param att A flag that specifies whether the output value is to be
        /// attenuated.
        void SetAttenuate (bool att)
        {
          m_attenuate = att;
        }

        /// Sets the position ( @a x, @a y, @a z ) of the end of the line
        /// segment to choose values along.
        ///
        /// @param x x coordinate of the end position.
        /// @param y y coordinate of the end position.
        /// @param z z coordinate of the end position.
        void SetEndPoint (real x, real y, real z)
        {
          m_x1 = x;
          m_y1 = y;
          m_z1 = z;
        }

        /// Sets the noise module that is used to generate the output values.
        ///
        /// @param module The noise module that is used to generate the output
        /// values.
        ///
        /// This noise module must exist for the lifetime of this object,
        /// until you pass a new noise module to this method.
        void SetModule (const module::Module& module)
        {
          m_pModule = &module;
        }

        /// Sets the position ( @a x, @a y, @a z ) of the start of the line
        /// segment to choose values along.
        ///
        /// @param x x coordinate of the start position.
        /// @param y y coordinate of the start position.
        /// @param z z coordinate of the start position.
        void SetStartPoint (real x, real y, real z)
        {
          m_x0 = x;
          m_y0 = y;
          m_z0 = z;
        }

      private:

        /// A flag that specifies whether the value is to be attenuated
        /// (moved toward 0.0) as the ends of the line segment are approached.
        bool m_attenuate;

        /// A pointer to the noise module used to generate the output values.
        const module::Module* m_pModule;

        /// @a x coordinate of the start of the line segment.
        real m_x0;

        /// @a x coordinate of the end of the line segment.
        real m_x1;

        /// @a y coordinate of the start of the line segment.
        real m_y0;

        /// @a y coordinate of the end of the line segment.
        real m_y1;

        /// @a z coordinate of the start of the line segment.
        real m_z0;

        /// @a z coordinate of the end of the line segment.
        real m_z1;

    };

    /// @}

    /// @}

  }

}

#endif
