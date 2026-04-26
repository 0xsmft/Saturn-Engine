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

#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"

#include <unordered_set>

namespace Saturn {

	class Sound;

	class SoundGraphTaskHandler : public NodeEditorTaskHandler
	{
	public:
		SoundGraphTaskHandler();
		virtual ~SoundGraphTaskHandler();

	public:
		virtual void Tick( Timestep ts ) override;

	public:
		void Loop( bool shouldLoop ) { m_Looping = shouldLoop; }

		void PlaySounds();
		void StopSounds();
		void DestroyAliveSounds();

		UUID AddNewSound( UUID assetID, bool spatialisation = false );

		void RegisterSound( size_t index );
		void UnregisterSound( size_t index );

		Ref<Sound> GetSoundFromIndex( size_t index );

	private:
		void OnSoundCompleted( UUID PlayerID );

	private:
		// Sounds that are currently playing
		std::vector<Ref<Sound>> m_AliveSounds;
		std::unordered_set<size_t> m_SoundsPlaying;

		bool m_Looping = false;
		bool m_Completed = false;
	};
	
}

