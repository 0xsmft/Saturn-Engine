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
#include "ShaderBundle.h"

#include "Saturn/Project/Project.h"
#include "Shader.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	struct ShaderBundleHeader
	{
		//									 .SB
		//								   | 4 BYTES			|		4 BYTES		    |
		const unsigned char Magic[ 4 ] = { 0x2E, 0x53, 0x42, 0x00/*, 0x00, 0x00, 0x00, 0x00*/ };
		size_t Shaders;
	};

	struct EditorShaderBundleHeader
	{
		//									 .SB				 .XA
		//								   | 4 BYTES			|		4 BYTES		    |
		const unsigned char Magic[ 8 ] = { 0x2E, 0x53, 0x42, 0x00, 0x2E, 0x58, 0x41, 0x00 };
		size_t Shaders;
	};

	static void WriteShaderBundleHeader( std::ofstream& rStream, ShaderBundleType flags ) 
	{
		switch( flags )
		{
			default:
			case ShaderBundleType::Normal:
			{
				ShaderBundleHeader header{};
				header.Shaders = ShaderLibrary::Get().GetShaders().size();
				rStream.write( reinterpret_cast< char* >( &header ), sizeof( ShaderBundleHeader ) );
			} break;
			
			case ShaderBundleType::Editor:
			{
				EditorShaderBundleHeader header{};
				header.Shaders = ShaderLibrary::Get().GetShaders().size();
				rStream.write( reinterpret_cast< char* >( &header ), sizeof( EditorShaderBundleHeader ) );
			} break;
		}
	}

	ShaderBundleResult ShaderBundle::BundleShaders( ShaderBundleType type )
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();
		
		// Editor shader bundle built into a different directory.
		if( type == ShaderBundleType::Editor )
		{
			cachePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle.ssb";
		}
		else
		{
			if( !std::filesystem::exists( cachePath ) )
				std::filesystem::create_directories( cachePath );

			cachePath /= "ShaderBundle.ssb";
		}

		std::ofstream fout( cachePath, std::ios::binary | std::ios::trunc );

		WriteShaderBundleHeader( fout, type );

		for( auto&& [name, shader] : ShaderLibrary::Get().GetShaders() )
		{
			SAT_CORE_INFO( "Packaging shader: {0}", name );

			size_t stringSize = name.length();
			fout.write( reinterpret_cast<char*>( &stringSize ), sizeof( size_t ) );

			fout.write( name.c_str(), stringSize );

			shader->SerialiseShaderData( fout );
		}

		fout.close();
	
		return ShaderBundleResult::Success;
	}

	static ShaderBundleResult ReadShaderBundleHeader( std::ifstream& rStream, ShaderBundleType type )
	{
		switch( type )
		{
			default:
			case ShaderBundleType::Normal:
			{
				ShaderBundleHeader header{};
				rStream.read( reinterpret_cast< char* >( &header ), sizeof( ShaderBundleHeader ) );

				// NB: Yes the header.Magic is 8 bytes but the real header is only 4
				// the remaining 4 bytes is just for padding and to make sure that
				if( std::memcmp( header.Magic, ".SB", 4 ) != 0 )
				{
					SAT_CORE_ERROR( "Invalid shader bundle file header!" );
					return ShaderBundleResult::InvalidShaderHeader;
				}
			} break;

			case ShaderBundleType::Editor:
				break;
		}

		return ShaderBundleResult::Success;
	}

	ShaderBundleResult ShaderBundle::ReadBundle( ShaderBundleType type )
	{
		std::filesystem::path cachePath = Project::GetActiveProject()->GetFullCachePath();
		if( type == ShaderBundleType::Editor )
		{
			cachePath = Application::Get()->GetAppDataFolder() / "EditorShaderBundle.ssb";
		}
		else
		{
			cachePath /= "ShaderBundle.ssb";
		}

		if( !std::filesystem::exists( cachePath ) )
			return ShaderBundleResult::FileNotFound;

		std::ifstream stream( cachePath, std::ios::binary | std::ios::in );

		size_t shaderCount = 0llu;
		switch( type )
		{
			default:
			case ShaderBundleType::Normal:
			{
				ShaderBundleHeader header{};
				stream.read( reinterpret_cast< char* >( &header ), sizeof( ShaderBundleHeader ) );

				// NB: Yes the header.Magic is 8 bytes but the real header is only 4
				// the remaining 4 bytes is just for padding and to make sure that
				if( std::memcmp( header.Magic, ".SB", 4 ) != 0 )
				{
					SAT_CORE_ERROR( "Invalid shader bundle file header!" );
					return ShaderBundleResult::InvalidShaderHeader;
				}

				shaderCount = header.Shaders;
			} break;
			
			case ShaderBundleType::Editor:
			{
				EditorShaderBundleHeader header{};
				stream.read( reinterpret_cast< char* >( &header ), sizeof( EditorShaderBundleHeader ) );

				if( std::memcmp( header.Magic, ".SB", 4 ) != 0 )
				{
					SAT_CORE_ERROR( "Invalid shader bundle file header!" );
					return ShaderBundleResult::InvalidShaderHeader;
				}

				if( std::memcmp( header.Magic + 4, ".XA", 4 ) != 0 )
				{
					SAT_CORE_ERROR( "Invalid shader bundle file header!" );
					return ShaderBundleResult::InvalidShaderHeader;
				}

				shaderCount = header.Shaders;
			} break;
		}

		for( size_t i = 0; i < shaderCount; i++ )
		{
			Ref<Shader> shader = Ref<Shader>::Create();

			std::string name = RawSerialisation::ReadString( stream );
			shader->m_Name = name;

			shader->DeserialiseShaderData( stream );

			ShaderLibrary::Get().Add( shader );
		}

		// We are done with the file buffer.
		stream.close();

		return ShaderBundleResult::Success;
	}
}
