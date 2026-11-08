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

	//
	// CameraShakeInstance
	// 
	// Local temporary instance of a CameraShakeDescriptorAsset.
	//
	// CameraShakeInstance can be created via a CameraShakeDescriptorAsset or from
	// raw values.
	// 
	// The reason why CameraShakeInstance exist is because CameraShakeDescriptorAsset is an Asset,
	// and assets can only be loaded once and global. So, to solve this we create a local instance
	// of the camera shake so that it can be played/stopped or even modified.
	//
	class CameraShakeInstance : public RefTarget
	{
	public:
		CameraShakeInstance( Ref<class CameraShakeDescriptorAsset> asset );
		CameraShakeInstance( float trauma, float decaySpeed, float maxPitchDeg, float maxYawDeg, float maxRollDeg );
		virtual ~CameraShakeInstance();

		glm::quat Tick( Timestep ts );
		void Reset();

	public:
		[[nodiscard]] bool IsCompleted() const { return m_Completed; }

	private:
		float m_Trauma = 0.0f;
		float m_CurrentTrauma = 0.0f;
		float m_DecaySpeed = 0.0f;

		float m_MaxPitchRad = 0.0f;
		float m_MaxYawRad = 0.0f;
		float m_MaxRollRad = 0.0f;

		bool m_Completed = false;
	};
	
}
