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

#include "Saturn/Core/Base.h"
#include "Saturn/Core/App.h"

#include "Saturn/GameFramework/ActionBinding.h"
#include "Saturn/Audio/SoundGroup.h"

#include "Saturn/Core/UUID.h"

#include <string>
#include <filesystem>

namespace Saturn {
	
	struct ProjectConfig
	{
		std::string Name;
		UUID StartupSceneID;

		// Relative path
		// "Assets"
		std::string AssetPath; 
		
		// Path to the .sproject file
		std::filesystem::path Path;
	};

	// Project
	//
	// Represents a .sproject file.
	// This is the highest level of the project chain
	// 
	// Project
	//  GameModule
	//   Game DLL file
	//    Game SObjects/SClass that get compiled into the ClassMetadataHandler
	// AssetManager relies on Project but is not owned or created by the project
	//
	// A Project MUST exist, if not, the editor will not startup
	//
	class Project : public RefTarget
	{
	public:
		// INTERNAL FOR USE BY PROJECT BROWSER ONLY!
		Project();
		Project( const ProjectConfig& rConfig );
		~Project();

		ProjectConfig& GetConfig() { return m_Config; }
		const ProjectConfig& GetConfig() const { return m_Config; }

		static ProjectConfig& GetActiveConfig() { return s_ActiveProject->m_Config; }

		// Local per module (copied to game when running in editor.)
		static Ref<Project> GetActiveProject();
		static void SetActiveProject( const Ref<Project>& rProject );

		// Only to be used by the Game.
		static std::filesystem::path FindProjectDir( const std::string& rName );

		void CheckMissingAssetRefs();

#if !defined(SAT_DIST)
		bool HasThumbnail() const;
		std::filesystem::path GetThumbnailPath() const;
#endif

	public:
		//////////////////////////////////////////////////////////////////////////
		// File and Project folder helpers.

		std::filesystem::path FilepathAbs( const std::filesystem::path& rPath ) const;

		// Relative Asset Path
		std::filesystem::path GetAssetPath() const;
		
		// Absolute Asset Path
		std::filesystem::path GetFullAssetPath() const;
		std::filesystem::path GetAbsoluteAssetPath() const { return GetFullAssetPath(); }
	
		// Relative Premake file
		std::filesystem::path GetPremakeFile() const;
		
		// Absolute project dir
		std::filesystem::path GetRootDir() const;
		
		// Absolute Temp dir
		std::filesystem::path GetTempDir() const;

		// Absolute source dir
		std::filesystem::path GetSourceDir() const;

		// Absolute bin dir
		std::filesystem::path GetBinDir();
		static std::filesystem::path GetActiveBinDir() { return s_ActiveProject->GetBinDir(); }

		// Absolute project dir (uses the project config's "Path" variable)
		std::filesystem::path GetProjectPath();
		static std::filesystem::path GetActiveProjectPath() { return s_ActiveProject->GetProjectPath(); }

		static std::filesystem::path GetActiveProjectRootPath() { return s_ActiveProject->GetRootDir(); }

		// Absolute cache dir
		std::filesystem::path GetFullCachePath() const;

		// Absolute App Data path for this project
		std::filesystem::path GetAppDataFolder() const;

	public:
		//////////////////////////////////////////////////////////////////////////
		// Action Bindings

		std::vector<ActionBindingData>& GetActionBindings() { return m_ActionBindings; }
		const std::vector<ActionBindingData>& GetActionBindings() const { return m_ActionBindings; }
		
		void AddActionBinding( const ActionBindingData& rBinding ) { m_ActionBindings.push_back( rBinding ); }
		void RemoveActionBinding( const ActionBindingData& rBinding );

		//////////////////////////////////////////////////////////////////////////
		// Sound Group

		std::vector<Ref<SoundGroup>>& GetSoundGroups() { return m_SoundGroups; }
		const std::vector<Ref<SoundGroup>>& GetSoundGroups() const { return m_SoundGroups; }

		void AddSoundGroup( const Ref<SoundGroup>& rGrp ) { m_SoundGroups.push_back( rGrp ); }
		void RemoveSoundGroup( const Ref<SoundGroup>& rGrp );

		//////////////////////////////////////////////////////////////////////////
		// Upgrading Assets

		void UpgradeAssets();

		//////////////////////////////////////////////////////////////////////////
		// Defaults

		const UUID GetDefaultMaterialAsset() const { return m_DefaultMaterialAsset; }
		void SetDefaultMaterialAsset( UUID newID ) { m_DefaultMaterialAsset = newID; }

		const UUID GetDefaultPhysicsMaterialAsset() const { return m_DefaultPhysicsMaterialAsset; }
		void SetDefaultPhysicsMaterialAsset( UUID newID ) { m_DefaultPhysicsMaterialAsset = newID; }

		//////////////////////////////////////////////////////////////////////////
		// Auto Saves, Editor only
#if !defined(SAT_DIST)
		inline float GetAutoSaveInterval() const { return m_AutoSaveInterval; }
		inline bool IsAutoSavesEnabled() const { return m_EnableAutoSaves; }

		inline void SetAutoSaveInterval( float interval ) { m_AutoSaveInterval = interval; }
		inline void EnableAutoSaves( bool value ) { m_EnableAutoSaves = value; }
#endif

	public:
		//////////////////////////////////////////////////////////////////////////
		// Premake, Building & Preparation for Distribution (Used in Editor)

		bool HasPremakeFile() const;
		void CreatePremakeFile( bool force = false ) const;
		void CreateBuildFile( bool force = false ) const;

		std::filesystem::path FindBuildTool() const;

		bool Build( ApplicationConfigKind kind, const std::string& rExtraArgs = "" );
		bool Rebuild( ApplicationConfigKind kind, const std::string& rExtraArgs = "" );
		void Distribute( ApplicationConfigKind kind, const std::string& rExtraArgs = "" );

		void PrepForDist() const;

	private:
		void CheckNewAssets();
		void CheckOfflineAssets();

	private:
		ProjectConfig m_Config;
		std::vector<ActionBindingData> m_ActionBindings;
		std::vector<Ref<SoundGroup>> m_SoundGroups;
		UUID m_DefaultMaterialAsset = 0;
		UUID m_DefaultPhysicsMaterialAsset = 0;
		
#if !defined(SAT_DIST)
		// Time in seconds, converted to minutes in the Editor, when changing the time
		float m_AutoSaveInterval = 300.0f;
		bool m_EnableAutoSaves = false;
#endif

		// Absolute root path
		std::filesystem::path m_RootPath;

#if !defined(SAT_DIST)
		std::filesystem::path m_ThumbnailImagePath;
#endif

	private:
		static inline Ref<Project> s_ActiveProject;
	};
}
