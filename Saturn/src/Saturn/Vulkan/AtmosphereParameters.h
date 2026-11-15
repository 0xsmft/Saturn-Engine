/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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

#pragma once

namespace Saturn {

	struct AtmosphereParameters
	{
		glm::vec3  AbsorptionColor{};
		float AbsorptionLength = 0.0f;

		glm::vec3  RayleighScatteringColor{};
		float RayleighScatterLength = 0.0f;

		glm::vec3  MieScatteringColor{};
		float MieScatteringLength = 0.0f;

		glm::vec3  MieAbsColor{};
		float MieAbsLength = 0.0f;

		glm::vec3 MieAbsorption{};
		int  ViewRayMarchMinSPP = 0;

		glm::vec3 GroundAlbedo{};
		int  ViewRayMarchMaxSPP = 0;

		glm::vec4 RayleighDensity[ 3 ]{};
		glm::vec4 MieDensity[ 3 ]{};
		glm::vec4 AbsorptionDensity[ 3 ]{};

		float MultipleScatteringFactor = 0.0f;
		float MiePhaseFunctionG = 0.0f;
		float BottomRadius = 0.0f;
		float TopRadius = 0.0f;
	};

}
