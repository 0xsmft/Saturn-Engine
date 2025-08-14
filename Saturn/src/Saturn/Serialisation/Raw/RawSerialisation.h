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

#include "Saturn/Core/Log.h"
#include "Saturn/Core/Base.h"
#include "Saturn/Core/Buffer.h"
#include "Saturn/Core/UUID.h"

#include <fstream>
#include <unordered_map>
#include <map>
#include <filesystem>

namespace Saturn {

#if !defined(SAT_DIST)
	// Determined by current build config, in Development configs read from a file
	using FDependentIStream = std::ifstream;
#else
	// In Dist, we read from a VFS file which is not an actual file so we can't use std::ifstream
	// VFS files are just a contentious span of file data
	using FDependentIStream = std::istream;
#endif

	template<typename Ty, typename OStream>
	concept HasSerialiseFunction = requires( const Ty& rObject, OStream& rStream )
	{
		{ Ty::Serialise( rObject, rStream ) } -> std::same_as<void>;
	};

	template<typename Ty, typename IStream>
	concept HasDeserialiseFunction = requires( Ty& rObject, IStream& rStream )
	{
		{ Ty::Deserialise( rObject, rStream ) } -> std::same_as<void>;
	};

	// Helpers for reading/writing in binary.
	class RawSerialisation
	{
	public:
		template<typename K, typename V, typename OStream>
		static void WriteUnorderedMap( const std::unordered_map<K, V>& rMap, OStream& rStream )
		{
			if( !rStream.is_open() )
				return;

			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& [key, value] : rMap )
			{
				WriteObjectChecked( key, rStream );
				WriteObjectChecked( value, rStream );
			}
		}

		// TODO: Move this into the VFS.
		// This is only used by the VFS.
		template<typename OStream>
		static void WriteUnorderedMap( const std::unordered_map<std::string, std::filesystem::path>& rMap, OStream& rStream )
		{
			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& [key, value] : rMap )
			{
				WriteString( key, rStream );
				WriteString( value.string(), rStream );
			}
		}

		template<typename K, typename V, typename OStream>
		static void WriteUnorderedMap( const std::unordered_map<K, std::vector<V>>& rMap, OStream& rStream )
		{
			if( !rStream.is_open() )
				return;

			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& [key, value] : rMap )
			{
				WriteObjectChecked( key, rStream );

				WriteVector( value, rStream );
			}
		}

		template<typename K, typename V, typename OStream>
		static void WriteMap( const std::map<K, V>& rMap, OStream& rStream )
		{
			if( !rStream.is_open() )
				return;

			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& [key, value] : rMap )
			{
				WriteObjectChecked( key, rStream );
				WriteObjectChecked( value, rStream );
			}
		}

		template<typename K, typename K2, typename V, typename OStream>
		static void WriteMap( const std::map<K, std::map<K2, V>>& rMap, OStream& rStream )
		{
			if( !rStream.is_open() )
				return;

			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& [key, value] : rMap )
			{
				// key
				WriteObjectChecked( key, rStream );

				// value
				WriteMap( value, rStream );
			}
		}

		// Used for maps with a string type as key and a filesystem path as K2
		template<typename V, typename OStream>
		static void WriteMap( const std::map<std::string, std::map<std::filesystem::path, V>>& rMap, OStream& rStream )
		{
			if( !rStream.is_open() )
				return;

			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& [key, value] : rMap )
			{
				WriteString( key, rStream );

				WriteMap( value, rStream );
			}
		}

		template<typename Ty, typename OStream>
		static void WriteVector( const std::vector<Ty>& rMap, OStream& rStream )
		{
			size_t mapSize = rMap.size();
			rStream.write( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( const auto& value : rMap )
			{
				WriteObjectChecked( value, rStream );
			}
		}

		// Write object (unchecked)
		template<typename Ty, typename OStream/*, std::enable_if_t<std::is_trivial<Ty>::value, bool> = true*/>
		static void WriteObject( const Ty& rObject, OStream& rStream )
		{
			rStream.write( reinterpret_cast< const char* >( &rObject ), sizeof( Ty ) );
		}

		// Attempts to find the most suitable serialisation method for this object based on it's traits
		// If Ty trival it will end up calling WriteObject()
		// If Ty is std::string or UUID, it will end up calling WriteString/WriteUUID
		// However, if it's none, it will end up calling Ty::Serialise
		template<typename Ty, typename OStream>
		static void WriteObjectChecked( const Ty& rObject, OStream& rStream )
		{
			// If trivial, write directly
			if constexpr( std::is_trivial<Ty>() )
			{
				WriteObject( rObject, rStream );
			}
			// If its a string, WriteString
			else if constexpr( std::is_same<Ty, std::string>() )
			{
				WriteString( rObject, rStream );
			}
			// If its a UUID, WriteUUID
			else if constexpr( std::is_same<Ty, Saturn::UUID>() )
			{
				WriteUUID( rObject, rStream );
			}
			// Fall back to defined "Serialise" function.
			else 
			{
				static_assert( HasSerialiseFunction<Ty, OStream>, "Ty is not trivial nor a std::string or a Saturn::UUID, in that case it must have a static void Serialise( const Ty&, std::ofstream )" );

				Ty::Serialise( rObject, rStream );
			}
		}

		// Read object (unchecked)
		template<typename Ty, typename IStream/*, std::enable_if_t<std::is_trivial<Ty>::value, bool> = true*/>
		static void ReadObject( Ty& rObject, IStream& rStream )
		{
			rStream.read( reinterpret_cast< char* >( &rObject ), sizeof( Ty ) );
		}

		// Attempts to find the most suitable deserialisation method for this object based on it's traits
		// If Ty trival it will end up calling ReadObject()
		// If Ty is std::string or UUID, it will end up calling ReadString/ReadUUID
		// However, if its none, it will end up calling Ty::Deserialise
		template<typename Ty, typename IStream>
		static void ReadObjectChecked( Ty& rObject, IStream& rStream )
		{
			// If trivial, read directly
			if constexpr( std::is_trivial<Ty>() )
			{
				ReadObject( rObject, rStream );
			}
			// If its a string, ReadString
			else if constexpr( std::is_same<Ty, std::string>() )
			{
				rObject = ReadString( rStream );
			}
			else if constexpr( std::is_same<Ty, Saturn::UUID>() )
			{
				ReadUUID( rObject, rStream );
			}
			else // fall back to defined "Deserialise" function
			{
				static_assert( HasDeserialiseFunction<Ty, IStream>, "Ty is not trivial nor a std::string or a Saturn::UUID, in that case it must have a static void Deserialise( <Ty>&, <FDependentIStream|std::ifstream> )" );

				Ty::Deserialise( rObject, rStream );
			}
		}

		template<typename OStream>
		static void WriteString( const std::string& rString, OStream& rStream )
		{
			size_t size = rString.size();
			rStream.write( reinterpret_cast< char* >( &size ), sizeof( size_t ) );

			rStream.write( rString.data(), size );
		}

		template<typename OStream>
		static void WriteString( const std::filesystem::path& rString, OStream& rStream )
		{
			const std::string stringbuf = rString.string();
			WriteString( stringbuf, rStream );
		}

		template<typename OStream>
		static void WriteString( const std::stringstream& rString, OStream& rStream )
		{
			const std::string stringbuf = rString.str();

			size_t size = stringbuf.size();
			rStream.write( reinterpret_cast< const char* >( &size ), sizeof( size ) );

			rStream.write( stringbuf.data(), size );
		}

		/////////////////////////////////////////////////////////////////////////
		// READING

		template<typename Ty, typename IStream>
		static void ReadVector( std::vector<Ty>& rMap, IStream& rStream )
		{
			if( rMap.size() )
				rMap.clear();

			size_t size = 0;
			rStream.read( reinterpret_cast< char* >( &size ), sizeof( size_t ) );
			rMap.resize( size );

			for( size_t i = 0; i < size; i++ )
			{
				Ty value{};
				ReadObjectChecked<Ty>( value, rStream );

				rMap[ i ] = value;
			}
		}

		template<typename K, typename V, typename IStream>
		static void ReadUnorderedMap( std::unordered_map<K, V>& rMap, IStream& rStream )
		{
			if( rMap.size() )
				rMap.clear();

			size_t size = 0;
			rStream.read( reinterpret_cast< char* >( &size ), sizeof( size_t ) );

			for( size_t i = 0; i < size; i++ )
			{
				K key{};
				ReadObjectChecked<K>( key, rStream );
				
				V value{};
				ReadObjectChecked<V>( value, rStream );
	
				rMap[ key ] = value;
			}
		}

		// TODO: Move this into the VFS.
		// This is only used by the VFS.
		template<typename IStream>
		static void ReadUnorderedMap( std::unordered_map<std::string, std::filesystem::path>& rMap, IStream& rStream )
		{
			size_t mapSize = 0;
			rStream.read( reinterpret_cast< char* >( &mapSize ), sizeof( size_t ) );

			for( size_t i = 0; i < mapSize; i++ )
			{
				std::string K{};
				std::filesystem::path V{};

				K = ReadString( rStream );
				V = ReadString( rStream );

				rMap[ K ] = V;
			}
		}

		template<typename K, typename V, typename IStream>
		static void ReadUnorderedMap( std::unordered_map<K, std::vector<V>>& rMap, IStream& rStream )
		{
			if( rMap.size() )
				rMap.clear();

			size_t size = 0;
			rStream.read( reinterpret_cast< char* >( &size ), sizeof( size_t ) );

			for( size_t i = 0; i < size; i++ )
			{
				K key{};
				ReadObjectChecked<K>( key, rStream );

				std::vector<V> values{};
				ReadVector( values, rStream );

				rMap[ key ] = std::move( values );
			}
		}

		template<typename K, typename V, typename IStream>
		static void ReadMap( std::map<K, V>& rMap, IStream& rStream )
		{
			if( rMap.size() )
				rMap.clear();

			size_t size = 0;
			rStream.read( reinterpret_cast< char* >( &size ), sizeof( size_t ) );

			for( size_t i = 0; i < size; i++ )
			{
				K key{};
				ReadObjectChecked<K>( key, rStream );

				V value{};
				ReadObjectChecked<V>( value, rStream );

				rMap[ key ] = value;
			}
		}

		template<typename IStream>
		[[nodiscard]] static std::string ReadString( IStream& rStream )
		{
			size_t length = 0;
			rStream.read( reinterpret_cast< char* >( &length ), sizeof( size_t ) );

			char* pTemporaryBuffer = new char[ length + 1 ];
			rStream.read( pTemporaryBuffer, length );

			pTemporaryBuffer[ length ] = '\0';

			std::string result = pTemporaryBuffer;

			delete[] pTemporaryBuffer;

			return result;
		}

		//////////////////////////////////////////////////////////////////////////
		// MATHS & SATURN BUFFERS

		template<typename OStream>
		static void WriteVec2( const glm::vec2& rVec, OStream& rStream )
		{
			WriteObject( rVec, rStream );
		}

		template<typename IStream>
		static void ReadVec2( glm::vec2& rVec, IStream& rStream )
		{
			ReadObject( rVec, rStream );
		}

		template<typename OStream>
		static void WriteVec3( const glm::vec3& rVec, OStream& rStream )
		{
			WriteObject( rVec, rStream );
		}

		template<typename IStream>
		static void ReadVec3( glm::vec3& rVec, IStream& rStream )
		{
			ReadObject( rVec, rStream );
		}

		template<typename OStream>
		static void WriteVec4( const glm::vec4& rVec, OStream& rStream )
		{
			WriteObject( rVec, rStream );
		}

		template<typename IStream>
		static void ReadVec4( glm::vec4& rVec, IStream& rStream )
		{
			ReadObject( rVec, rStream );
		}

		template<typename OStream>
		static void WriteMatrix4x4( const glm::mat4& rMat, OStream& rStream )
		{
			WriteObject( rMat, rStream );
		}

		template<typename IStream>
		static void ReadMatrix4x4( glm::mat4& rMat, IStream& rStream )
		{
			ReadObject( rMat, rStream );
		}

		template<typename OStream>
		static void WriteSaturnBuffer( Buffer& rBuffer, OStream& rStream )
		{
			rStream.write( reinterpret_cast<char*>( &rBuffer.Size ), sizeof( size_t ) );
			rStream.write( reinterpret_cast<char*>( rBuffer.Data ), rBuffer.Size );
		}

		template<typename OStream>
		static void WriteSaturnBuffer( const Buffer& rBuffer, OStream& rStream )
		{
			rStream.write( reinterpret_cast< const char* >( &rBuffer.Size ), sizeof( size_t ) );
			rStream.write( reinterpret_cast< const char* >( rBuffer.Data ), rBuffer.Size );
		}

		template<typename IStream>
		static void ReadSaturnBuffer( Buffer& rBuffer, IStream& rStream )
		{
			rBuffer.Free();

			size_t BufferSize = 0;

			rStream.read( reinterpret_cast< char* >( &BufferSize ), sizeof( size_t ) );
			
			uint8_t* pData = new uint8_t[ BufferSize ];
			rStream.read( reinterpret_cast< char* >( pData ), BufferSize );

			rBuffer = Buffer::Copy( pData, BufferSize );

			delete[] pData;
		}

		//////////////////////////////////////////////////////////////////////////
		// UUID

		template<typename OStream>
		static void WriteUUID( const Saturn::UUID& rUUID, OStream& rStream )
		{
			RawSerialisation::WriteObject( (uint64_t)rUUID, rStream );
		}

		template<typename IStream>
		static void ReadUUID( Saturn::UUID& rUUID, IStream& rStream )
		{
			uint64_t id = 0;
			RawSerialisation::ReadObject( id, rStream );

			rUUID.SetID( id );
		}
	};
}
