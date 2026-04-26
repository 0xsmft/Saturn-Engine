/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 BEAST                                                           *
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

#include "SoundGroup.h"
#include "SoundBase.h"

#include "Saturn/NodeEditor/NodeTaskCache.h"

#include "Saturn/Asset/Asset.h"

namespace Saturn {

	class SoundGraphTaskHandler;

	class GraphSound : public SoundBase
	{
	public:
		GraphSound( AssetID id );
		~GraphSound();

		void Initialise();

		virtual void Play( uint64_t frameOffset = 0llu ) override;
		virtual void Stop() override;
		virtual void Loop( bool loop = true ) override;
		virtual void Load( uint32_t flags ) override;
		virtual void Reset() override;
		virtual void SetVolume( float volume ) override;
		virtual void SetPitch( float pitch ) override;
		virtual void SetSpatialisation( bool value ) override;

		// Public non-static version of OnSoundEnd
		// OnSoundCompleted is called by the Audio System
		void OnSoundCompleted() override;

		void WaitUntilLoaded();

		void SetPosition( const glm::vec3& rPos );

	private:
		void Unload() override;

	private:
		// The "GraphSound" class is not an asset however GraphSounds are an asset
		Ref<Asset> m_GraphAsset;
		Ref<SoundGroup> m_SoundGroup;
		Ref<SoundGraphTaskHandler> m_TaskHandler;
	
		// Temporary: will be moved into a centralised storage.
		NodeTaskCache m_TaskCache;
	};
}
