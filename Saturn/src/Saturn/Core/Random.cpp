/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
*                                                                                           *
* Permission is hereby granted, free of charge, to any person obtaining a copy              *
* of this software and associated documentation files (the "Software"), to deal             *
* in the Software without restriction, including without limitation the rights              *
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell                 *
* copies of the Software, and to permit persons to whom the Software is                     *
* furnished to do so, subject to the following conditions:                                  *
*                                                                                           *
* The above copyright notice and this permission notice shall be included in all            *
* copies or substantial portions of the Software.                                           *
*                                                                                           *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR                *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,                  *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE               *
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER                    *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,             *
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE             *
* SOFTWARE.                                                                                 *
*********************************************************************************************
*/

#include "sppch.h"
#include "Random.h"

#include <random>

namespace Saturn {

	static std::random_device s_RandomDevice;
	static std::mt19937_64 s_RandomEngine( s_RandomDevice() );
	static std::uniform_int_distribution<uint64_t> s_UniformDistribution;

	bool Random::RandomBool()
	{
		static std::uniform_int_distribution<int> s_BooleanDistribution( 0, 1 );

		return s_BooleanDistribution( s_RandomEngine );
	}

	uint64_t Random::RandomUUID()
	{
		return s_UniformDistribution( s_RandomEngine );
	}

	size_t Random::RandomElementInRange( size_t min, size_t max )
	{
		std::uniform_int_distribution<size_t> rangeDistribution( min, max );

		return rangeDistribution( s_RandomEngine );
	}

}