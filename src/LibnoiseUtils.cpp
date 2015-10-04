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


//////////////////////////////////////////////////////////////////////////////
// Miscellaneous functions

namespace noise
{

    namespace utils
    {

        // Performs linear interpolation between two 8-bit channel values.
        inline noise::uint8 BlendChannel(const uint8 channel0,
                                         const uint8 channel1, float alpha)
        {
            float c0 = (float)channel0 / 255.0f;
            float c1 = (float)channel1 / 255.0f;
            return (noise::uint8)(((c1 * alpha) + (c0 * (1.0f - alpha))) * 255.0f);
        }

        // Performs linear interpolation between two colors and stores the result
        // in out.
        inline void LinearInterpColor(const Color& color0, const Color& color1,
                                      float alpha, Color& out)
        {
            out.alpha = BlendChannel(color0.alpha, color1.alpha, alpha);
            out.blue  = BlendChannel(color0.blue , color1.blue , alpha);
            out.green = BlendChannel(color0.green, color1.green, alpha);
            out.red   = BlendChannel(color0.red  , color1.red  , alpha);
        }

        // Unpacks a floating-point value into four bytes.  This function is
        // specific to Intel machines.  A portable version will come soon (I
        // hope.)
        inline noise::uint8* UnpackFloat(noise::uint8* bytes, float value)
        {
            noise::uint8* pBytes = (noise::uint8*)(&value);
            bytes[0] = *pBytes++;
            bytes[1] = *pBytes++;
            bytes[2] = *pBytes++;
            bytes[3] = *pBytes++;
            return bytes;
        }

        // Unpacks a 16-bit integer value into two bytes in little endian format.
        inline noise::uint8* UnpackLittle16(noise::uint8* bytes,
                                            noise::uint16 integer)
        {
            bytes[0] = (noise::uint8)((integer & 0x00ff));
            bytes[1] = (noise::uint8)((integer & 0xff00) >> 8);
            return bytes;
        }

        // Unpacks a 32-bit integer value into four bytes in little endian format.
        inline noise::uint8* UnpackLittle32(noise::uint8* bytes,
                                            noise::uint32 integer)
        {
            bytes[0] = (noise::uint8)((integer & 0x000000ff));
            bytes[1] = (noise::uint8)((integer & 0x0000ff00) >> 8);
            bytes[2] = (noise::uint8)((integer & 0x00ff0000) >> 16);
            bytes[3] = (noise::uint8)((integer & 0xff000000) >> 24);
            return bytes;
        }

    }

}

using namespace noise;

using namespace noise::utils;

//////////////////////////////////////////////////////////////////////////////
// GradientColor class

GradientColor::GradientColor()
{
    m_pGradientPoints = NULL;
}

GradientColor::~GradientColor()
{
    delete[] m_pGradientPoints;
}

void GradientColor::AddGradientPoint(NOISE_REAL gradientPos,
                                     const noise::utils::Color& gradientColor)
{
    // Find the insertion point for the new gradient point and insert the new
    // gradient point at that insertion point.  The gradient point array will
    // remain sorted by gradient position.
    int insertionPos = FindInsertionPos(gradientPos);
    InsertAtPos(insertionPos, gradientPos, gradientColor);
}

void GradientColor::Clear()
{
    delete[] m_pGradientPoints;
    m_pGradientPoints = NULL;
    m_gradientPointCount = 0;
}

int GradientColor::FindInsertionPos(NOISE_REAL gradientPos)
{
    int insertionPos;
    for (insertionPos = 0; insertionPos < m_gradientPointCount;
         insertionPos++)
    {
        if (gradientPos < m_pGradientPoints[insertionPos].pos)
        {
            // We found the array index in which to insert the new gradient point.
            // Exit now.
            break;
        }
        else if (gradientPos == m_pGradientPoints[insertionPos].pos)
        {
            // Each gradient point is required to contain a unique gradient
            // position, so throw an exception.
            throw noise::ExceptionInvalidParam();
        }
    }
    return insertionPos;
}

const noise::utils::Color& GradientColor::GetColor(NOISE_REAL gradientPos) const
{
    assert(m_gradientPointCount >= 2);

    // Find the first element in the gradient point array that has a gradient
    // position larger than the gradient position passed to this method.
    int indexPos;
    for (indexPos = 0; indexPos < m_gradientPointCount; indexPos++)
    {
        if (gradientPos < m_pGradientPoints[indexPos].pos)
        {
            break;
        }
    }

    // Find the two nearest gradient points so that we can perform linear
    // interpolation on the color.
    int index0 = ClampValue(indexPos - 1, 0, m_gradientPointCount - 1);
    int index1 = ClampValue(indexPos    , 0, m_gradientPointCount - 1);

    // If some gradient points are missing (which occurs if the gradient
    // position passed to this method is greater than the largest gradient
    // position or less than the smallest gradient position in the array), get
    // the corresponding gradient color of the nearest gradient point and exit
    // now.
    if (index0 == index1)
    {
        m_workingColor = m_pGradientPoints[index1].color;
        return m_workingColor;
    }

    // Compute the alpha value used for linear interpolation.
    NOISE_REAL input0 = m_pGradientPoints[index0].pos;
    NOISE_REAL input1 = m_pGradientPoints[index1].pos;
    NOISE_REAL alpha = (gradientPos - input0) / (input1 - input0);

    // Now perform the linear interpolation given the alpha value.
    const Color& color0 = m_pGradientPoints[index0].color;
    const Color& color1 = m_pGradientPoints[index1].color;
    LinearInterpColor(color0, color1, (float)alpha, m_workingColor);
    return m_workingColor;
}

void GradientColor::InsertAtPos(int insertionPos, NOISE_REAL gradientPos,
	const noise::utils::Color& gradientColor)
{
    // Make room for the new gradient point at the specified insertion position
    // within the gradient point array.  The insertion position is determined by
    // the gradient point's position; the gradient points must be sorted by
    // gradient position within that array.
    GradientPoint* newGradientPoints;
    newGradientPoints = new GradientPoint[m_gradientPointCount + 1];
    for (int i = 0; i < m_gradientPointCount; i++)
    {
        if (i < insertionPos)
        {
            newGradientPoints[i] = m_pGradientPoints[i];
        }
        else
        {
            newGradientPoints[i + 1] = m_pGradientPoints[i];
        }
    }
    delete[] m_pGradientPoints;
    m_pGradientPoints = newGradientPoints;
    ++m_gradientPointCount;

    // Now that we've made room for the new gradient point within the array, add
    // the new gradient point.
    m_pGradientPoints[insertionPos].pos = gradientPos ;
    m_pGradientPoints[insertionPos].color = gradientColor;
}

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

//////////////////////////////////////////////////////////////////////////////
// Image class

noise::utils::Image::Image()
{
    InitObj();
}

noise::utils::Image::Image(int width, int height)
{
    InitObj();
    SetSize(width, height);
}

noise::utils::Image::Image(const noise::utils::Image& rhs)
{
    InitObj();
    CopyImage(rhs);
}

noise::utils::Image::~Image()
{
    delete[] m_pImage;
}

noise::utils::Image& noise::utils::Image::operator= (const noise::utils::Image& rhs)
{
    CopyImage(rhs);

    return *this;
}

void noise::utils::Image::Clear(const noise::utils::Color& value)
{
    if (m_pImage != NULL)
    {
        for (int y = 0; y < m_height; y++)
        {
            Color* pDest = GetSlabPtr(0, y);
            for (int x = 0; x < m_width; x++)
            {
                *pDest++ = value;
            }
        }
    }
}

void noise::utils::Image::CopyImage(const noise::utils::Image& source)
{
    // Resize the image buffer, then copy the slabs from the source image
    // buffer to this image buffer.
    SetSize(source.GetWidth(), source.GetHeight());
    for (int y = 0; y < source.GetHeight(); y++)
    {
        const Color* pSource = source.GetConstSlabPtr(0, y);
        Color* pDest = GetSlabPtr(0, y);
        memcpy(pDest, pSource, (size_t)source.GetWidth() * sizeof(float));
    }

    // Copy the border value as well.
    m_borderValue = source.m_borderValue;
}

void noise::utils::Image::DeleteImageAndReset()
{
    delete[] m_pImage;
    InitObj();
}

noise::utils::Color noise::utils::Image::GetValue(int x, int y) const
{
    if (m_pImage != NULL)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            return *(GetConstSlabPtr(x, y));
        }
    }
    // The coordinates specified are outside the image.  Return the border
    // value.
    return m_borderValue;
}

void noise::utils::Image::InitObj()
{
    m_pImage  = NULL;
    m_height  = 0;
    m_width   = 0;
    m_stride  = 0;
    m_memUsed = 0;
    m_borderValue = Color(0, 0, 0, 0);
}

void noise::utils::Image::ReclaimMem()
{
    size_t newMemUsage = CalcMinMemUsage(m_width, m_height);
    if (m_memUsed > newMemUsage)
    {
        // There is wasted memory.  Create the smallest buffer that can fit the
        // data and copy the data to it.
        Color* pNewImage = NULL;
        try
        {
            pNewImage = new Color[newMemUsage];
        }
        catch (...)
        {
            throw noise::ExceptionOutOfMemory();
        }
        memcpy(pNewImage, m_pImage, newMemUsage * sizeof(float));
        delete[] m_pImage;
        m_pImage = pNewImage;
        m_memUsed = newMemUsage;
    }
}

void noise::utils::Image::SetSize(int width, int height)
{
    if (width < 0 || height < 0
        || width > RASTER_MAX_WIDTH || height > RASTER_MAX_HEIGHT)
    {
        // Invalid width or height.
        throw noise::ExceptionInvalidParam();
    }
    else if (width == 0 || height == 0)
    {
        // An empty image was specified.  Delete it and zero out the size member
        // variables.
        DeleteImageAndReset();
    }
    else
    {
        // A new image size was specified.  Allocate a new image buffer unless
        // the current buffer is large enough for the new image (we don't want
        // costly NOISE_REALlocations going on.)
        size_t newMemUsage = CalcMinMemUsage(width, height);
        if (m_memUsed < newMemUsage)
        {
            // The new size is too big for the current image buffer.  We need to
            // NOISE_REALlocate.
            DeleteImageAndReset();
            try
            {
                m_pImage = new Color[newMemUsage];
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

void noise::utils::Image::SetValue(int x, int y, const noise::utils::Color& value)
{
    if (m_pImage != NULL)
    {
        if (x >= 0 && x < m_width && y >= 0 && y < m_height)
        {
            *(GetSlabPtr(x, y)) = value;
        }
    }
}

void noise::utils::Image::TakeOwnership(noise::utils::Image& source)
{
    // Copy the values and the image buffer from the source image to this image.
    // Now this image pwnz the source buffer.
    delete[] m_pImage;
    m_memUsed = source.m_memUsed;
    m_height  = source.m_height;
    m_pImage  = source.m_pImage;
    m_stride  = source.m_stride;
    m_width   = source.m_width;

    // Now that the source buffer is assigned to this image, reset the source
    // image object.
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

