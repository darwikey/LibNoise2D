// noiseutils.cpp
//
// Copyright (C) 2003-2005 Jason Bevins
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

#include <fstream>

#include <interp.h>
#include <mathconsts.h>

#include "LibnoiseUtils.h"

using namespace noise;
using namespace noise::model;
using namespace noise::module;


using namespace noise;

using namespace noise::utils;


//////////////////////////////////////////////////////////////////////////////
// NoiseMap class

NoiseMap::NoiseMap()
{
    InitObj();
}

NoiseMap::NoiseMap(int width, int height)
{
    InitObj();
    SetSize(width, height);
}

NoiseMap::NoiseMap(const NoiseMap& rhs)
{
    InitObj();
    CopyNoiseMap(rhs);
}

NoiseMap::~NoiseMap()
{
    delete[] m_pNoiseMap;
}

NoiseMap& NoiseMap::operator= (const NoiseMap& rhs)
{
    CopyNoiseMap(rhs);

    return *this;
}

void NoiseMap::Clear(float value)
{
    if (m_pNoiseMap != NULL)
    {
        for (int y = 0; y < m_height; y++)
        {
            float* pDest = GetSlabPtr(0, y);
            for (int x = 0; x < m_width; x++)
            {
                *pDest++ = value;
            }
        }
    }
}

void NoiseMap::CopyNoiseMap(const NoiseMap& source)
{
    // Resize the noise map buffer, then copy the slabs from the source noise
    // map buffer to this noise map buffer.
    SetSize(source.GetWidth(), source.GetHeight());
    for (int y = 0; y < source.GetHeight(); y++)
    {
        const float* pSource = source.GetConstSlabPtr(0, y);
        float* pDest = GetSlabPtr(0, y);
        memcpy(pDest, pSource, (size_t)source.GetWidth() * sizeof(float));
    }

    // Copy the border value as well.
    m_borderValue = source.m_borderValue;
}

void NoiseMap::DeleteNoiseMapAndReset()
{
    delete[] m_pNoiseMap;
    InitObj();
}

float NoiseMap::GetValue(int x, int y) const
{
    if (m_pNoiseMap != NULL)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            return *(GetConstSlabPtr(x, y));
        }
    }
    // The coordinates specified are outside the noise map.  Return the border
    // value.
    return m_borderValue;
}

void NoiseMap::InitObj()
{
    m_pNoiseMap = NULL;
    m_height    = 0;
    m_width     = 0;
    m_stride    = 0;
    m_memUsed   = 0;
    m_borderValue = 0.0;
}

void NoiseMap::ReclaimMem()
{
    size_t newMemUsage = CalcMinMemUsage(m_width, m_height);
    if (m_memUsed > newMemUsage)
    {
        // There is wasted memory.  Create the smallest buffer that can fit the
        // data and copy the data to it.
        float* pNewNoiseMap = NULL;
        try
        {
            pNewNoiseMap = new float[newMemUsage];
        }
        catch (...)
        {
            throw noise::ExceptionOutOfMemory();
        }
        memcpy(pNewNoiseMap, m_pNoiseMap, newMemUsage * sizeof(float));
        delete[] m_pNoiseMap;
        m_pNoiseMap = pNewNoiseMap;
        m_memUsed = newMemUsage;
    }
}

void NoiseMap::SetSize(int width, int height)
{
    if (width < 0 || height < 0
        || width > RASTER_MAX_WIDTH || height > RASTER_MAX_HEIGHT)
    {
        // Invalid width or height.
        throw noise::ExceptionInvalidParam();
    }
    else if (width == 0 || height == 0)
    {
        // An empty noise map was specified.  Delete it and zero out the size
        // member variables.
        DeleteNoiseMapAndReset();
    }
    else
    {
        // A new noise map size was specified.  Allocate a new noise map buffer
        // unless the current buffer is large enough for the new noise map (we
        // don't want costly NOISE_REALlocations going on.)
        size_t newMemUsage = CalcMinMemUsage(width, height);
        if (m_memUsed < newMemUsage)
        {
            // The new size is too big for the current noise map buffer.  We need to
            // NOISE_REALlocate.
            DeleteNoiseMapAndReset();
            try
            {
                m_pNoiseMap = new float[newMemUsage];
            }
            catch (...)
            {
                throw noise::ExceptionOutOfMemory();
            }
            m_memUsed = newMemUsage;
        }
        m_stride = (int)CalcStride(width);
        m_width  = width;
        m_height = height;
    }
}

void NoiseMap::SetValue(int x, int y, float value)
{
    if (m_pNoiseMap != NULL)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            *(GetSlabPtr(x, y)) = value;
        }
    }
}

void NoiseMap::TakeOwnership(NoiseMap& source)
{
    // Copy the values and the noise map buffer from the source noise map to
    // this noise map.  Now this noise map pwnz the source buffer.
    delete[] m_pNoiseMap;
    m_memUsed   = source.m_memUsed;
    m_height    = source.m_height;
    m_pNoiseMap = source.m_pNoiseMap;
    m_stride    = source.m_stride;
    m_width     = source.m_width;

    // Now that the source buffer is assigned to this noise map, reset the
    // source noise map object.
    source.InitObj();
}


void NoiseMapBuilder::Build()
{
    if (m_upperXBound <= m_lowerXBound
        || m_upperZBound <= m_lowerZBound
        || m_destWidth <= 0
        || m_destHeight <= 0
        || m_pSourceModule == NULL
        || m_pDestNoiseMap == NULL)
    {
        throw noise::ExceptionInvalidParam();
    }

    // Resize the destination noise map so that it can store the new output
    // values from the source model.
    m_pDestNoiseMap->SetSize(m_destWidth, m_destHeight);

    // Create the plane model.
    model::Plane planeModel;
    planeModel.SetModule(*m_pSourceModule);

    NOISE_REAL xExtent = m_upperXBound - m_lowerXBound;
    NOISE_REAL zExtent = m_upperZBound - m_lowerZBound;
    NOISE_REAL xDelta  = xExtent / (NOISE_REAL)m_destWidth ;
    NOISE_REAL zDelta  = zExtent / (NOISE_REAL)m_destHeight;
    NOISE_REAL xCur    = m_lowerXBound;
    NOISE_REAL zCur    = m_lowerZBound;

    // Fill every point in the noise map with the output values from the model.
    for (int z = 0; z < m_destHeight; z++)
    {
        float* pDest = m_pDestNoiseMap->GetSlabPtr(z);
        xCur = m_lowerXBound;
        for (int x = 0; x < m_destWidth; x++)
        {
            float finalValue;
            if (!m_isSeamlessEnabled)
            {
                finalValue = (float)planeModel.GetValue(xCur, zCur);
            }
            else
            {
                NOISE_REAL swValue, seValue, nwValue, neValue;
                swValue = planeModel.GetValue(xCur          , zCur);
                seValue = planeModel.GetValue(xCur + xExtent, zCur);
                nwValue = planeModel.GetValue(xCur          , zCur + zExtent);
                neValue = planeModel.GetValue(xCur + xExtent, zCur + zExtent);
                NOISE_REAL xBlend = 1.0f - ((xCur - m_lowerXBound) / xExtent);
                NOISE_REAL zBlend = 1.0f - ((zCur - m_lowerZBound) / zExtent);
                NOISE_REAL z0 = LinearInterp(swValue, seValue, xBlend);
                NOISE_REAL z1 = LinearInterp(nwValue, neValue, xBlend);
                finalValue = (float)LinearInterp(z0, z1, zBlend);
            }
            *pDest++ = finalValue;
            xCur += xDelta;
        }
        zCur += zDelta;
        
    }
}


void NoiseMapBuilder::Build(std::function<void(int, int, float)> fCallback)
{
	if (m_upperXBound <= m_lowerXBound
		|| m_upperZBound <= m_lowerZBound
		|| m_destWidth <= 0
		|| m_destHeight <= 0
		|| m_pSourceModule == NULL)
	{
		throw noise::ExceptionInvalidParam();
	}

	//m_pDestNoiseMap->SetSize(m_destWidth, m_destHeight);

	// Create the plane model.
	model::Plane planeModel;
	planeModel.SetModule(*m_pSourceModule);

	NOISE_REAL xExtent = m_upperXBound - m_lowerXBound;
	NOISE_REAL zExtent = m_upperZBound - m_lowerZBound;
	NOISE_REAL xDelta = xExtent / (NOISE_REAL)m_destWidth;
	NOISE_REAL zDelta = zExtent / (NOISE_REAL)m_destHeight;
	NOISE_REAL xCur = m_lowerXBound;
	NOISE_REAL zCur = m_lowerZBound;

	// Fill every point in the noise map with the output values from the model.
	for (int z = 0; z < m_destHeight; z++)
	{
		xCur = m_lowerXBound;
		for (int x = 0; x < m_destWidth; x++)
		{
			float finalValue;
			if (!m_isSeamlessEnabled)
			{
				finalValue = (float)planeModel.GetValue(xCur, zCur);
			}
			else
			{
				NOISE_REAL swValue, seValue, nwValue, neValue;
				swValue = planeModel.GetValue(xCur, zCur);
				seValue = planeModel.GetValue(xCur + xExtent, zCur);
				nwValue = planeModel.GetValue(xCur, zCur + zExtent);
				neValue = planeModel.GetValue(xCur + xExtent, zCur + zExtent);
				NOISE_REAL xBlend = 1.0f - ((xCur - m_lowerXBound) / xExtent);
				NOISE_REAL zBlend = 1.0f - ((zCur - m_lowerZBound) / zExtent);
				NOISE_REAL z0 = LinearInterp(swValue, seValue, xBlend);
				NOISE_REAL z1 = LinearInterp(nwValue, neValue, xBlend);
				finalValue = (float)LinearInterp(z0, z1, zBlend);
			}
			// callback
			fCallback(x, z, finalValue);

			xCur += xDelta;
		}
		zCur += zDelta;

	}
}

