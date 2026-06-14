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

	struct EditorShaderBundleHeader
	{
		//									.LA
		const unsigned char Magic[ 4 ] = { 0x2E, 0x4C, 0x41, 0x00 };
		size_t Shaders = 0llu;
	};

	static void WriteHeader( std::ofstream& rStream ) 
	{
		EditorShaderBundleHeader header{};
		header.Shaders = ShaderLibrary::Get().GetShaderCount();

		rStream.write( reinterpret_cast< const char* >( &header.Magic ), 4 );
		rStream.write( reinterpret_cast< const char* >( &header.Shaders ), sizeof( size_t ) );
	}

	static bool ReadHeader( EditorShaderBundleHeader& header, std::ifstream& rStream )
	{
		unsigned char magic[ 4 ]{ 0 };

		rStream.read( reinterpret_cast< char* >( &magic ), 4 );
		rStream.read( reinterpret_cast< char* >( &header.Shaders ), sizeof( size_t ) );

		if( std::memcmp( magic, ".LA", 4 ) != 0 )
		{
			return false;
		}

		return true;
	}

	bool EditorShaderBundle::BundleShaders()
	{
		const std::filesystem::path cachePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle.ssb";

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		WriteHeader( fout );

		for( auto&& [name, shader] : ShaderLibrary::Get().GetShaders() )
		{
			SAT_CORE_INFO( "Packaging shader: {0}", name );

			const auto& path = shader->GetFilepath();
			const auto lwt = std::filesystem::last_write_time( path );
			const auto epochTime = lwt.time_since_epoch().count();

			RawSerialisation::WriteObject( epochTime, fout );

			shader->SerialiseShaderDataForEditor( fout );
		}

		fout.close();

		return true;
	}

	bool EditorShaderBundle::ReadBundle()
	{
		const std::filesystem::path cachePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle.ssb";

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		EditorShaderBundleHeader header{};
		if( !ReadHeader( header, stream ) )
			return false;

		for( size_t i = 0; i < header.Shaders; ++i )
		{
			std::chrono::system_clock::rep savedLastWriteTime = 0ll;
			RawSerialisation::ReadObject( savedLastWriteTime, stream );

			Ref<Shader> shader = Ref<Shader>::Create();
			shader->DeserialiseShaderDataForEditor( stream );

			const auto lwt = std::filesystem::last_write_time( shader->GetFilepath() );
			const auto fsLastWriteTime = lwt.time_since_epoch().count();

			// This is a bit hacky but will work fine.
			// so, if our times match we add it to the library
			// if not we do, then not we do nothing because when the shader 
			// is needed it will then load it from the .glsl file
			// because it does not exist in the map.
			// 
			if( fsLastWriteTime == savedLastWriteTime )
			{
				ShaderLibrary::Get().Add( shader );
			}
		}

		stream.close();

		return true;
	}

}
