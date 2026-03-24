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

#include "sppch.h"
#include "Project.h"

#include "Saturn/Core/EngineSettings.h"
#include "Saturn/Core/EnvironmentVariables.h"
#include "Saturn/Core/StringAuxiliary.h"
#include "Saturn/Core/Process.h"

#include "Saturn/Serialisation/YAML/EngineSettingsSerialiser.h"
#include "Saturn/Serialisation/YAML/ProjectSerialiser.h"
#include "Saturn/Serialisation/YAML/AssetManagerSerialiser.h"

#include "Saturn/Asset/AssetManager.h"

#include "SharedGlobals.h"

namespace Saturn {
	
	static const std::vector<std::string> s_AllowedAssetExtentions
	{
		{ ".stx"          },
		{ ".snd"          },
		{ ".gsnd"         },
		{ ".scene"        },
		{ ".smaterial"    },
		{ ".cpp"          },
		{ ".h"            },
		{ ".prefab"       },
		{ ".sphymaterial" }
	};

	Project::Project()
	{
	}

	Project::Project( const ProjectConfig& rConfig )
		: m_Config( rConfig )
	{
		m_RootPath = m_Config.Path.parent_path();

#if !defined(SAT_DIST)
		m_ThumbnailImagePath = GetFullCachePath() / "PerUser";

		if( !std::filesystem::exists( m_ThumbnailImagePath ) )
			std::filesystem::create_directories( m_ThumbnailImagePath );

		m_ThumbnailImagePath /= "Thumbnail.png";
#endif
	}

	Project::~Project()
	{
	}

	Ref<Project> Project::GetActiveProject()
	{
		return s_ActiveProject;
	}

	void Project::SetActiveProject( const Ref<Project>& rProject )
	{
		s_ActiveProject = rProject;
	}

	void Project::CheckNewAssets()
	{
		bool FileChanged = false;
		auto AssetPath = GetFullAssetPath();

		for( auto& rEntry : std::filesystem::recursive_directory_iterator( AssetPath ) )
		{
			if( rEntry.is_directory() )
				continue;

			std::filesystem::path filepath = std::filesystem::relative( rEntry.path(), GetRootDir() );
			auto filepathString = filepath.extension().string();

			if( filepath.extension() == ".sreg" )
				continue;

			Ref<Asset> asset = AssetManager::Get()->FindAsset( filepath );

			if( std::find( s_AllowedAssetExtentions.begin(), s_AllowedAssetExtentions.end(), filepathString ) == s_AllowedAssetExtentions.end() )
				continue; // Extension is forbidden.

			if( asset == nullptr )
			{
				SAT_CORE_INFO( "Found an asset that exists in the system filesystem, however not in the asset registry, creating new asset." );

				// Add to pending file list.
				// Editor will show dialog and handle the rest.

				auto type = ExtensionToAssetType( filepathString );
				auto id = AssetManager::Get()->CreateAsset( type );
				asset = AssetManager::Get()->FindAsset( id );

				asset->SetAbsolutePath( rEntry.path() );

				FileChanged = true;
			}
		}

		if( FileChanged )
		{
			AssetManagerSerialiser ars;
			ars.Serialise();
		}
	}

	void Project::CheckOfflineAssets()
	{
		std::vector<AssetID> pendingAssets;

		bool FileChanged = false;

		auto& assetReg = AssetManager::Get()->GetAssetRegistry()->GetAssetMap();
		for( auto& [id, rAsset] : assetReg )
		{
			if( !rAsset )
				continue;

			if( std::filesystem::exists( FilepathAbs( rAsset->Path ) ) )
				continue;

			SAT_CORE_WARN( "Found an asset that is present in the Asset Registry however no longer exists in the filesystem, removing from Asset Registry..." );

			pendingAssets.push_back( id );
		}

		for( const auto& id : pendingAssets )
		{
			AssetManager::Get()->RemoveAsset( id );
		}

		if( FileChanged )
		{
			AssetManagerSerialiser ars;
			ars.Serialise();
		}
	}

	void Project::CheckMissingAssetRefs()
	{
		CheckOfflineAssets();
		CheckNewAssets();
	}

#if !defined(SAT_DIST)
	bool Project::HasThumbnail() const
	{
		return std::filesystem::exists( m_ThumbnailImagePath );
	}

	std::filesystem::path Project::GetThumbnailPath() const
	{
		return m_ThumbnailImagePath;
	}
#endif // !SAT_DIST

	std::filesystem::path Project::GetFullAssetPath() const
	{
		// Root dir
		std::filesystem::path rootDir = m_RootPath;
		rootDir /= "Assets";

		return rootDir;
	}

	std::filesystem::path Project::GetPremakeFile() const
	{
		return "premake5.lua";
	}

	std::filesystem::path Project::GetRootDir() const
	{
		return m_RootPath;
	}

	std::filesystem::path Project::GetTempDir() const
	{
		return m_RootPath / "Temp";
	}

	std::filesystem::path Project::GetSourceDir() const
	{
		return m_RootPath / "Source" / m_Config.Name;
	}

	std::filesystem::path Project::GetBinDir()
	{
		auto rootDir = GetRootDir();
		rootDir /= "bin";

		rootDir /= std::format( "{0}-{1}", Application::Get()->GetCurrentConfigName(), SAT_PLATFORM_BINARY_FOLDER );
		rootDir /= m_Config.Name;

		return rootDir;
	}

	std::filesystem::path Project::GetProjectPath()
	{
		return GetActiveProject()->GetConfig().Path;
	}

	std::filesystem::path Project::FilepathAbs( const std::filesystem::path& rPath ) const
	{
		std::filesystem::path rootDir = m_RootPath;
		rootDir /= rPath;

		return rootDir;
	}

	std::filesystem::path Project::GetFullCachePath() const
	{
		return m_RootPath / "Cache";
	}

	std::filesystem::path Project::GetAppDataFolder() const
	{
		std::filesystem::path appData = Application::Get()->GetAppDataFolder();

		return appData /= m_Config.Name;
	}

	void Project::RemoveActionBinding( const ActionBindingData& rBinding )
	{
		m_ActionBindings.erase( std::remove( m_ActionBindings.begin(), m_ActionBindings.end(), rBinding ), m_ActionBindings.end() );
	}

	void Project::AddSoundGroup( const Ref<SoundGroup>& rGrp )
	{
		m_SoundGroups.push_back( rGrp );
	}

	void Project::RemoveSoundGroup( const Ref<SoundGroup>& rGrp )
	{
		m_SoundGroups.erase( std::remove( m_SoundGroups.begin(), m_SoundGroups.end(), rGrp ), m_SoundGroups.end() );
	}

	void Project::UpgradeAssets()
	{
		// TODO: For now this function will just update the "Version" variable in the asset
		//       In the future we will want to actually upgrade the asset.

		AssetManager::Get()->BumpAssetVersion( SAT_CURRENT_VERSION );
		
		AssetManagerSerialiser ars;
		ars.Serialise();
	}

	void Project::RemoveAssetFromDefaults(/*AssetID*/ UUID assetID )
	{
		if( m_Config.StartupSceneID == assetID )
			m_Config.StartupSceneID = 0llu;

		if( m_DefaultMaterialAsset == assetID )
			m_DefaultMaterialAsset = 0llu;

		if( m_DefaultPhysicsMaterialAsset == assetID )
			m_DefaultPhysicsMaterialAsset = 0llu;

		if( m_DefaultFontAsset == assetID )
			m_DefaultFontAsset = 0llu;
	}

	bool Project::HasPremakeFile() const
	{
		return std::filesystem::exists( m_RootPath / "premake5.lua" );
	}

	void Project::CreatePremakeFile( bool force ) const
	{
		const auto PremakePath = m_RootPath / "premake5.lua";

		if( std::filesystem::exists( PremakePath ) && !force )
			return;

		std::filesystem::copy( "content/Templates/premake5.lua", PremakePath, std::filesystem::copy_options::overwrite_existing );

		std::ifstream ifs( PremakePath );

		std::string fileData;
		if( ifs )
		{
			ifs.seekg( 0, std::ios_base::end );
			const auto size = static_cast< size_t >( ifs.tellg() );
			ifs.seekg( 0, std::ios_base::beg );

			fileData.reserve( size );
			fileData.assign( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
		}

		size_t pos = fileData.find( "__PROJECT_NAME__" );
		while( pos != std::string::npos )
		{
			fileData.replace( pos, std::strlen( "__PROJECT_NAME__" ), m_Config.Name.c_str() );

			pos = fileData.find( "__PROJECT_NAME__" );
		}

		std::ofstream fout( PremakePath );
		fout << fileData;
	}

	void Project::TryCopyCSharpTargetFiles( bool force ) const
	{
		// Copy over the development target file.
		auto BuildFilePath = GetRootDir() / "Source";
		BuildFilePath /= m_Config.Name + ".Development.cs";

		if( !std::filesystem::exists( BuildFilePath ) || force ) 
		{
			std::filesystem::copy( "content/Templates/%PROJECT_NAME%.Development.cs", BuildFilePath, std::filesystem::copy_options::overwrite_existing );

			ReplaceProjectNameTokens( BuildFilePath );
		}

		// Copy over the distribution target file.
		BuildFilePath = GetRootDir() / "Source";
		BuildFilePath /= m_Config.Name + ".Dist.cs";

		if( !std::filesystem::exists( BuildFilePath ) || force )
		{
			std::filesystem::copy( "content/Templates/%PROJECT_NAME%.Dist.cs", BuildFilePath, std::filesystem::copy_options::overwrite_existing );

			ReplaceProjectNameTokens( BuildFilePath );
		}

		// Copy over the module file.
		BuildFilePath = GetRootDir() / "Source";
		BuildFilePath /= m_Config.Name + ".Module.cs";

		if( !std::filesystem::exists( BuildFilePath ) || force )
		{
			std::filesystem::copy( "content/Templates/%PROJECT_NAME%.Module.cs", BuildFilePath, std::filesystem::copy_options::overwrite_existing );

			ReplaceProjectNameTokens( BuildFilePath );
		}
	}

	void Project::ReplaceProjectNameTokens( const std::filesystem::path& rPath ) const
	{
		std::ifstream ifs( rPath );

		std::string fileData;
		if( ifs )
		{
			ifs.seekg( 0, std::ios_base::end );
			auto size = static_cast< size_t >( ifs.tellg() );
			ifs.seekg( 0, std::ios_base::beg );

			fileData.reserve( size );
			fileData.assign( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
		}

		const std::string projectName = m_Config.Name;

		size_t pos = fileData.find( "%PROJECT_NAME%" );
		while( pos != std::string::npos )
		{
			fileData.replace( pos, std::strlen( "%PROJECT_NAME%" ), projectName );
			pos = fileData.find( "%PROJECT_NAME%" );
		}

		std::ofstream fout( rPath );
		fout << fileData;
	}

	void Project::PrepForDist() const
	{
		// Copy over the runtime build file.
		auto BuildFilePath = GetRootDir() / "Source";
		BuildFilePath /= m_Config.Name + ".Distribution.cs";

		if( !std::filesystem::exists( BuildFilePath ) )
			std::filesystem::copy( "content/Templates/%PROJECT_NAME%.Distribution.cs", BuildFilePath );

		// Copy over the client main file
		auto BuildPath = GetFullAssetPath().parent_path() / "Build";
		BuildPath /= m_Config.Name + ".Entry.cpp";

		if( std::filesystem::exists( BuildPath ) )
			std::filesystem::remove( BuildPath );

		std::filesystem::create_directory( GetFullAssetPath().parent_path() / "Build" );

		std::filesystem::copy( "content/Templates/%PROJECT_NAME%.Entry.cpp", BuildPath );

		std::ifstream ifs( BuildPath );

		std::string fileData;

		if( ifs )
		{
			ifs.seekg( 0, std::ios_base::end );
			auto size = static_cast< size_t >( ifs.tellg() );
			ifs.seekg( 0, std::ios_base::beg );

			fileData.reserve( size );
			fileData.assign( std::istreambuf_iterator<char>( ifs ), std::istreambuf_iterator<char>() );
		}

		size_t pos = fileData.find( "%PROJECT_NAME%" );

		while( pos != std::string::npos )
		{
			std::string projectPath = m_Config.Name;
			std::replace( projectPath.begin(), projectPath.end(), '\\', '/' );

			fileData.replace( pos, 14, projectPath );

			pos = fileData.find( "%PROJECT_NAME%" );
		}

		std::ofstream fout( BuildPath );
		fout << fileData;
	}

	std::filesystem::path Project::FindBuildTool() const
	{
		const std::filesystem::path SaturnRootDir = Auxiliary::GetEnvironmentVariable( "SATURN_DIR" );
		std::filesystem::path BuildToolDir = SaturnRootDir;

		BuildToolDir /= "bin";
		BuildToolDir /= std::format( "{0}-{1}", Application::Get()->GetCurrentConfigName(), SAT_PLATFORM_BINARY_FOLDER );
		BuildToolDir /= "SaturnBuildTool";

#if defined( SAT_PLATFORM_WINDOWS )
		BuildToolDir /= "SaturnBuildTool.exe";
#elif defined( SAT_PLATFORM_LINUX )
		BuildToolDir /= "SaturnBuildTool";
#else
		BuildToolDir /= "SaturnBuildTool.app";
#endif

		return BuildToolDir;
	}

	bool Project::Build( ApplicationConfigKind kind, const std::string& rExtraArgs )
	{
		std::filesystem::path BuildToolDir = FindBuildTool();

		std::filesystem::path WorkingDir = BuildToolDir.parent_path();

		std::string Args = BuildToolDir.string();

		Args += " /BUILD /NAME:";
		
		Args += m_Config.Name;
		
		Args += " /WIN64";
		
		switch( kind )
		{
			case Saturn::ApplicationConfigKind::Debug:
				Args += " /DEBUG /PROJECT:";
				break;
		
			case Saturn::ApplicationConfigKind::Release:
				Args += " /RELEASE /PROJECT:";
				break;
			
			case Saturn::ApplicationConfigKind::Dist:
				Args += " /DIST /PROJECT:";
				break;
		}
	
		Args += GetRootDir().string();

		Args += " " + rExtraArgs;
		std::wstring wArgs = Auxiliary::ConvertString( Args );

		// Start the process
		Process buildTool( wArgs, WorkingDir );

		int exitCode = buildTool.ResultOfProcess();

		return exitCode == 0;
	}

	bool Project::Rebuild( ApplicationConfigKind kind, const std::string& rExtraArgs )
	{
		std::filesystem::path BuildToolDir = FindBuildTool();

		std::filesystem::path WorkingDir = BuildToolDir.parent_path();

		std::string Args = BuildToolDir.string();

		Args += " /REBUILD /NAME:";

		Args += m_Config.Name;

		Args += " /WIN64";

		switch( kind )
		{
			case Saturn::ApplicationConfigKind::Debug:
				Args += " /DEBUG /PROJECT:";
				break;

			case Saturn::ApplicationConfigKind::Release:
				Args += " /RELEASE /PROJECT:";
				break;

			case Saturn::ApplicationConfigKind::Dist:
				Args += " /DIST /PROJECT:";
				break;
		}

		Args += GetRootDir().string();

		Args += " " + rExtraArgs;
		std::wstring wArgs = Auxiliary::ConvertString( Args );

		// Start the process
		Process buildTool( wArgs, WorkingDir );

		int exitCode = buildTool.ResultOfProcess();

		return exitCode == 0;
	}

	void Project::Distribute( ApplicationConfigKind kind, const std::string& rExtraArgs )
	{
		std::filesystem::path SaturnBinDir = Auxiliary::GetEnvironmentVariable( "SATURN_DIR" );
		std::filesystem::path binDir = GetRootDir();

		SaturnBinDir /= "bin";
		binDir /= "bin";
		
		switch( kind )
		{
			case Saturn::ApplicationConfigKind::Debug:
				SaturnBinDir /= std::format( "Debug-{0}", SAT_PLATFORM_BINARY_FOLDER );
				binDir /= std::format( "Debug-{0}", SAT_PLATFORM_BINARY_FOLDER );
				break;

				// Use editor release DLLs -- same as release ones
			case Saturn::ApplicationConfigKind::Dist:
			case Saturn::ApplicationConfigKind::Release:
				SaturnBinDir /= std::format( "Release-{0}", SAT_PLATFORM_BINARY_FOLDER );
				binDir /= std::format( "Release-{0}", SAT_PLATFORM_BINARY_FOLDER );
				break;
		}

		// We use the editor because the editor will have the DLLs that we need to copy over.
		SaturnBinDir /= "Saturn-Editor";
		binDir /= m_Config.Name;

		for( const auto& rEntry : std::filesystem::directory_iterator( SaturnBinDir ) )
		{
			auto& path = rEntry.path();

			// TODO: Change for other platforms.
			if( path.extension() != ".dll" ) 
				continue;

			std::filesystem::path dstPath = binDir / path.filename();

			if( std::filesystem::exists( dstPath ) )
				std::filesystem::remove( dstPath );

			std::filesystem::copy_file( path, dstPath );
		}

		std::filesystem::path dstCache = binDir / "Cache";

		if( std::filesystem::exists( dstCache ) )
			std::filesystem::remove_all( dstCache );

		std::filesystem::copy( GetRootDir() / "Cache", dstCache, std::filesystem::copy_options::recursive );

		// Copy the project file over
		std::filesystem::path projectFilePathBin = binDir / std::filesystem::path( m_Config.Name ).replace_extension(".sproject");

		if( std::filesystem::exists( projectFilePathBin ) )
			std::filesystem::remove( projectFilePathBin );

		std::filesystem::copy_file( m_Config.Path, projectFilePathBin );
		
		// TEMP: Copy over the editor assets
		std::filesystem::path contentDir = Application::Get()->GetRootContentDir();
		
		if( std::filesystem::exists( binDir / "content" ) )
			std::filesystem::remove_all( binDir / "content" );

		std::filesystem::copy( contentDir, binDir / "content", std::filesystem::copy_options::recursive );
	}
}
