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

#pragma once

#include "SingletonStorage.h"
#include "Saturn/Core/Thread.h"

#include "AudioCore.h"
#include "Sound.h"
#include "SoundGroup.h"
#include "GraphSound.h"

#include <filesystem>

namespace Saturn {

	enum AudioType
	{
		Static, // 2D
		Dynamic, // 3D
		Unknown
	};

	class AudioThread : public Thread
	{
	public:
		AudioThread();
		virtual ~AudioThread();

	public:
		virtual void Start() override;
		virtual void RequestJoin() override;

	private:
		void ThreadRun();

		bool m_PendingTerminate = false;
	};

	class Entity;

	class AudioSystem
	{
	public:
		SAT_SINGLETON_LAZY( AudioSystem )

	public:
		AudioSystem();
		~AudioSystem();

		void Terminate();
		void WaitForInit();

		// Loads a new Sound2D to be played
		// By default the sound will automatically play upon loading.
		// WARNING:
		//  This function uses the Audio Thread
		//  This function will return the sound however, it may not be loaded as soon as it returns!
		//  Use WaitUntilLoaded for safety.
		Ref<Sound> RequestNewSound( AssetID ID, UUID UniquePlayerID = Saturn::UUID(), bool PlayNow = true, Ref<SoundGroup> soundGroup = nullptr );
		Ref<Sound> PlaySoundAtLocation( AssetID ID, UUID UniquePlayerID, const glm::vec3& rPos, bool PlayNow = true, Ref<SoundGroup> soundGroup = nullptr );
	
		Ref<Sound> RequestPreviewSound( AssetID AssetID, UUID Identifier, bool allowMultiple = false );

		Ref<GraphSound> PlayGraphSound( AssetID ID, UUID UniquePlayerID );

		void RequestNewSounds( std::vector<AssetID> Ids, std::vector<UUID> PlayerIds, std::function<void(Ref<Sound>)>&& rVistor );

		void ReportSoundCompleted( UUID UniquePlayerID );

		void Suspend();
		void Resume();

		void SetPrimaryListenerPos( const glm::vec3& rPos );
		void SetPrimaryListenerDirection( const glm::vec3& rDir );

		void StopPreviewSounds( UUID Identifier );
		void StopAndResetSound( UUID UniquePlayerID );
		void StopSound( UUID UniquePlayerID );

		void UnloadSound( Ref<SoundBase> sound );
		void UnloadSound( UUID UniquePlayerID );

		void StartSoundGroups();
		void StopSoundGroups();

		ma_engine& GetAudioEngine() { return m_Engine; }
		Ref<SoundGroup>& GetMasterSoundGroup() { return m_MasterSoundGroup; }

		SoundDecodedInformation DecodeSound( const Ref<SoundSpecification>& rSpec );

		Ref<Sound> FindSound( UUID UniquePlayerID );

		Ref<AudioThread> GetThread() { return m_AudioThread; }

	private:
		void Initialise();
		void PlaySound( Ref<Sound> rSoundAsset );

	private:
		Ref<AudioThread> m_AudioThread;
		Ref<SoundGroup> m_MasterSoundGroup;

		// Currently alive sounds (i.e. sounds that are playing)
		std::unordered_map<UUID, Ref<SoundBase>> m_AliveSounds;
		// A list of every loaded sound in memory.
		std::unordered_map<UUID, Ref<SoundBase>> m_LoadedSounds;

		// Preview sound are not available in Dist.
#if defined( SAT_DEBUG ) || defined( SAT_RELEASE )
		// Identifier -> Asset ID -> Sound
		std::unordered_map<UUID, std::unordered_map< AssetID, Ref<SoundBase> >> m_PreviewSounds;
#endif

	private:
		ma_engine m_Engine;
		ma_context m_Context;
		ma_device m_Device;

		std::atomic<bool> m_Initialised = false;
	};
}