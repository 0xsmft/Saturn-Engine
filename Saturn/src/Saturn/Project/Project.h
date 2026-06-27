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

#include "SaturnBuildToolExitCodes.h"

#include "Saturn/Core/Base.h"
#include "Saturn/Core/App.h"

#include "Saturn/GameFramework/ActionBinding.h"
#include "Saturn/Audio/SoundGroup.h"

#include "Saturn/Online/OnlineSystemAPIType.h"

#include "Saturn/Core/UUID.h"

#include <string>
#include <filesystem>

namespace Saturn {
	
	struct ProjectConfig
	{
		std::string Name;
		UUID StartupSceneID;

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
	//    Game SObjects/SClass that get compiled "into" ClassMetadataHandler
	//
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

		void CheckMissingAssetRefs();

#if !defined(SAT_DIST)
		bool HasThumbnail() const;
		std::filesystem::path GetThumbnailPath() const;
#endif

	public:
		//////////////////////////////////////////////////////////////////////////
		// File and Project folder helpers.

		std::filesystem::path FilepathAbs( const std::filesystem::path& rPath ) const;

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

		void AddSoundGroup( const Ref<SoundGroup>& rGrp );
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

		const UUID GetDefaultFontAsset() const { return m_DefaultFontAsset; }
		void SetDefaultFontAsset( UUID newID ) { m_DefaultFontAsset = newID; }

		const UUID GetDefaultPhysRegAsset() const { return m_DefaultPhysSurfaceRegistryAsset; }
		void SetDefaultPhysRegAsset( UUID newID ) { m_DefaultPhysSurfaceRegistryAsset = newID; }

		void RemoveAssetFromDefaults( /*AssetID*/ UUID assetID );

		//////////////////////////////////////////////////////////////////////////
		// Auto Saves, Editor only
#if !defined(SAT_DIST)
		inline float GetAutoSaveInterval() const { return m_AutoSaveInterval; }
		inline bool IsAutoSavesEnabled() const { return m_EnableAutoSaves; }

		inline void SetAutoSaveInterval( float interval ) { m_AutoSaveInterval = interval; }
		inline void EnableAutoSaves( bool value ) { m_EnableAutoSaves = value; }
#endif

		//////////////////////////////////////////////////////////////////////////
		// Information, Editor only
#if !defined(SAT_DIST)
		inline std::string GetDeveloperVersion() const { return m_DeveloperProjectVersion; }

		inline void SetDeveloperVersion( const std::string& ver ) { m_DeveloperProjectVersion = ver; }
#endif

		//////////////////////////////////////////////////////////////////////////
		// Online System API

		OnlineSystemAPIType GetOnlineAPIType() const { return m_OnlineAPIType; }
		void SetOnlineSystemAPI( OnlineSystemAPIType type ) { m_OnlineAPIType = type; }

		void SetOnlineAppID( uint32_t id ) { m_OnlineAppID = id; }
		uint32_t GetOnlineAppID() const { return m_OnlineAppID; }

	public:
		//////////////////////////////////////////////////////////////////////////
		// Premake, Building & Preparation for Distribution (Used in Editor)

		bool HasPremakeFile() const;
		void CreatePremakeFile( bool force = false ) const;
		void TryCopyCSharpTargetFiles( bool force = false ) const;

		std::filesystem::path FindBuildTool() const;

		[[nodiscard]] SaturnBuildToolExitCodes Build( ApplicationConfigKind kind, const std::string& rExtraArgs = "" );
		[[nodiscard]] SaturnBuildToolExitCodes Rebuild( ApplicationConfigKind kind, const std::string& rExtraArgs = "" );
		void Distribute( ApplicationConfigKind kind, const std::string& rExtraArgs = "" );

		void PrepForDist() const;

	private:
		void CheckNewAssets();
		void CheckOfflineAssets();
		void ReplaceProjectNameTokens( const std::filesystem::path& rPath ) const;

	private:
		ProjectConfig m_Config;
		std::vector<ActionBindingData> m_ActionBindings;
		std::vector<Ref<SoundGroup>> m_SoundGroups;
		UUID m_DefaultMaterialAsset = 0;
		UUID m_DefaultPhysicsMaterialAsset = 0;
		UUID m_DefaultFontAsset = 0;
		UUID m_DefaultPhysSurfaceRegistryAsset = 0;

		OnlineSystemAPIType m_OnlineAPIType = OnlineSystemAPIType::Steam;

		// TODO: This makes sense to be in the project, however, should be suited to be in the OnlineSystem.
		//		 Fine for now, or forever...
		uint32_t m_OnlineAppID = 0u;
		
#if !defined(SAT_DIST)
		// Time in seconds, converted to minutes or any suitable time to display in the Editor, when changing the time
		float m_AutoSaveInterval = 300.0f;
		bool m_EnableAutoSaves = false;
#endif

		// Absolute root path
		std::filesystem::path m_RootPath;

#if !defined(SAT_DIST)
		std::filesystem::path m_ThumbnailImagePath;
		// This is the version that is local to the developer, it has nothing to do with Saturn versions.
		// It is purely informative.
		std::string m_DeveloperProjectVersion;
#endif

	private:
		static inline Ref<Project> s_ActiveProject;
	};
}
