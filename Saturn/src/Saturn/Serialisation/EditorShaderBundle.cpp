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
#include "EditorShaderBundle.h"

#include "Saturn/Core/App.h"

#include "Saturn/Vulkan/Shader.h"

#include "Raw/RawSerialisation.h"

namespace Saturn {

	static bool s_PendingWrite = false;

	enum class EditorShaderBundleVersion
	{
		BeforeVersionWasAdded = 0,
		ShaderDependencies = 1,

		Lowest = BeforeVersionWasAdded,
		Latest = ShaderDependencies
	};

	struct EditorShaderBundleHeader
	{
		//									.LA
		const unsigned char Magic[ 3 ] = { 0x2E, 0x4C, 0x41 };
		EditorShaderBundleVersion EditorBundleVersion = EditorShaderBundleVersion::Lowest;
		size_t Shaders = 0llu;
	};

	static void WriteHeader( std::ofstream& rStream ) 
	{
		EditorShaderBundleHeader header{};
		header.Shaders = ShaderLibrary::Get().GetShaderCount();
		header.EditorBundleVersion = EditorShaderBundleVersion::Latest;

		rStream.write( reinterpret_cast< const char* >( &header.Magic ), 3 );
		rStream.write( reinterpret_cast< const char* >( &header.EditorBundleVersion ), 1 );
		rStream.write( reinterpret_cast< const char* >( &header.Shaders ), sizeof( size_t ) );
	}

	static bool ReadHeader( EditorShaderBundleHeader& header, std::ifstream& rStream )
	{
		unsigned char magic[ 3 ]{ 0 };

		rStream.read( reinterpret_cast< char* >( &magic ), 3 );
		rStream.read( reinterpret_cast< char* >( &header.EditorBundleVersion ), 1 );
		rStream.read( reinterpret_cast< char* >( &header.Shaders ), sizeof( size_t ) );

		if( std::memcmp( magic, ".LA", 3 ) != 0 )
		{
			return false;
		}

		return true;
	}

	template<typename RepType = std::chrono::milliseconds::rep>
	static RepType GetLastWriteTimeUnixTimeMs( const std::filesystem::path& rPath ) 
	{
		const auto lwt = std::filesystem::last_write_time( rPath );

		// Seems like AppleClang does not support clock_cast
		// and using lwt.time_since_epoch() works fine on macOS
		// but on Windows we get the wrong time
		// so using a clock_cast is needed because I want all
		// shaders to be the unix epoch time and not the system file clock time.
#if defined(SAT_PLATFORM_WINDOWS) || defined(SAT_COMPILER_MSVC)
		const auto systemTime = std::chrono::clock_cast< std::chrono::system_clock >( lwt );
		const auto unixTimeMs = std::chrono::duration_cast< std::chrono::milliseconds >( systemTime.time_since_epoch() ).count();
#else
		const auto unixTimeMs = std::chrono::duration_cast< std::chrono::milliseconds >( lwt.time_since_epoch() ).count();
#endif

		return unixTimeMs;
	}

	bool EditorShaderBundle::BundleShaders()
	{
#if defined(SAT_DIST)
		return false;
#else
		const std::filesystem::path cachePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle-" SAT_CURRENT_VERSION_BUILD_TAG ".ssb";

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		WriteHeader( fout );

		for( auto&& [name, shader] : ShaderLibrary::Get().GetShaders() )
		{
			SAT_CORE_INFO( "Packaging shader: {0}", name );

			const auto& rShaderPath = shader->GetFilepath();
			
			const auto unixTimeMs = GetLastWriteTimeUnixTimeMs( rShaderPath );
			RawSerialisation::WriteObject( unixTimeMs, fout );

			shader->SerialiseShaderDataForEditor( fout );

			// Shader dependencies
			const auto shaderDependencies = shader->GetShaderDependencies();

			RawSerialisation::WriteObject( shaderDependencies.size(), fout );
			
			for( const auto& rDep : shaderDependencies )
			{
				RawSerialisation::WriteString( rDep.string(), fout );

				const auto depTimeMs = GetLastWriteTimeUnixTimeMs( rDep );
				RawSerialisation::WriteObject( depTimeMs, fout );
			}
		}

		fout.close();

		return true;
#endif
	}

	bool EditorShaderBundle::ReadBundle()
	{
#if defined(SAT_DIST)
		return false;
#else
		const std::filesystem::path cachePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle-" SAT_CURRENT_VERSION_BUILD_TAG ".ssb";

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		EditorShaderBundleHeader header{};
		if( !ReadHeader( header, stream ) )
		{
			// If we failed to read, then we know the editor will re-package it so make sure we mark this as a pending write.
			s_PendingWrite = true;
			return false;
		}

		for( size_t i = 0; i < header.Shaders; ++i )
		{
			uint64_t savedLastWriteTime = 0llu;
			RawSerialisation::ReadObject( savedLastWriteTime, stream );

			Ref<Shader> shader = Ref<Shader>::Create();
			shader->DeserialiseShaderDataForEditor( stream );

			if( !std::filesystem::exists( shader->GetFilepath() ) )
			{
				SAT_CORE_WARN( "Skipping shader: {} as the file path does not exist!", shader->GetName() );
				continue;
			}

			bool anyDependenciesModified = false;
			if( header.EditorBundleVersion >= EditorShaderBundleVersion::ShaderDependencies )
			{
				size_t mapSize = 0llu;
				RawSerialisation::ReadObject( mapSize, stream );

				shader->m_ShaderDependencies.reserve( mapSize );

				for( size_t i = 0; i < mapSize; ++i )
				{
					std::string dep = RawSerialisation::ReadString( stream );
					shader->m_ShaderDependencies.push_back( dep );

					std::chrono::milliseconds::rep time = std::chrono::milliseconds::rep( 0 );
					RawSerialisation::ReadObject( time, stream );

					const auto fsDepTime = GetLastWriteTimeUnixTimeMs( dep );
					anyDependenciesModified |= ( fsDepTime != time );
				}
			}

			const auto fsLastWriteTimeMs = GetLastWriteTimeUnixTimeMs( shader->GetFilepath() );
			if( ( fsLastWriteTimeMs != savedLastWriteTime ) || anyDependenciesModified )
			{
				s_PendingWrite = true;
			}
			else
			{
				// This is a bit hacky but will work fine.
				// so, if our times match we add it to the library
				// if not we do, then not we do nothing because when the shader 
				// is needed it will then load it from the .glsl file
				// because it does not exist in the map.
				// 
				ShaderLibrary::Get().Add( shader );
			}
		}

		stream.close();

		return true;
#endif
	}

	void EditorShaderBundle::MarkForceWrite()
	{
		s_PendingWrite = true;
	}

	void EditorShaderBundle::TryPackageIfNeeded()
	{
		if( s_PendingWrite )
		{
			if( BundleShaders() )
			{
				s_PendingWrite = false;
			}
			else
				SAT_CORE_WARN( "Failed to package editor shader bundle!" );
		}
	}

}
