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

#include "Saturn/Core/Memory/Buffer.h"

#include <fstream>
#include <unordered_map>
#include <map>
#include <filesystem>

namespace Saturn {

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
				if constexpr( std::is_trivial<K>() )
				{
					WriteObject( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						WriteString( key, rStream );
					}
					else
					{
						K::Serialise( key, rStream );
					}
				}

				if constexpr( std::is_trivial<V>() )
				{
					WriteObject( value, rStream );
				}
				else
				{
					if constexpr( std::is_same<V, std::string>() )
					{
						WriteString( value, rStream );
					}
					else
					{
						V::Serialise( value, rStream );
					}
				}
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
				if constexpr( std::is_trivial<K>() )
				{
					WriteObject( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						WriteString( key, rStream );
					}
					else
					{
						K::Serialise( key, rStream );
					}
				}

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
				if constexpr( std::is_trivial<K>() )
				{
					WriteObject( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						WriteString( key, rStream );
					}
					else
					{
						K::Serialise( key, rStream );
					}
				}

				if constexpr( std::is_trivial<V>() )
				{
					WriteObject( value, rStream );
				}
				else
				{
					if constexpr( std::is_same<V, std::string>() )
					{
						WriteString( value, rStream );
					}
					else
					{
						V::Serialise( value, rStream );
					}
				}
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
				if constexpr( std::is_trivial<K>() )
				{
					WriteObject( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						WriteString( key, rStream );
					}
					else
					{
						K::Serialise( key, rStream );
					}
				}

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
				if constexpr( std::is_trivial<Ty>() )
				{
					WriteObject( value, rStream );
				}
				else
				{
					if constexpr( std::is_same<Ty, std::string>() )
					{
						WriteString( value, rStream );
					}
					else
					{
						Ty::Serialise( value, rStream );
					}
				}
			}
		}

		template<typename Ty, typename OStream>
		static void WriteObject( const Ty& rObject, OStream& rStream )
		{
			rStream.write( reinterpret_cast< const char* >( &rObject ), sizeof( Ty ) );
		}

		template<typename Ty, typename IStream>
		static void ReadObject( Ty& rObject, IStream& rStream )
		{
			rStream.read( reinterpret_cast<char*>( &rObject ), sizeof( Ty ) );
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
			std::string stringbuf = rString.string();
			WriteString( stringbuf, rStream );
		}

		template<typename OStream>
		static void WriteString( const std::stringstream& rString, OStream& rStream )
		{
			std::string stringbuf = rString.str();

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
				if constexpr( std::is_trivial<Ty>() )
				{
					ReadObject<Ty>( value, rStream );
				}
				else
				{
					if constexpr( std::is_same<Ty, std::string>() )
					{
						value = ReadString( rStream );
					}
					else
					{
						Ty::Deserialise( value, rStream );
					}
				}

				rMap[i] = value;
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
				if constexpr( std::is_trivial<K>() )
				{
					ReadObject<K>( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						key = ReadString( rStream );
					}
					else
					{
						K::Deserialise( key, rStream );
					}
				}

				V value{};
				if constexpr( std::is_trivial<V>() )
				{
					ReadObject<V>( value, rStream );
				}
				else
				{
					if constexpr( std::is_same<V, std::string>() )
					{
						value = ReadString( rStream );
					}
					else
					{
						V::Deserialise( value, rStream );
					}
				}

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
				if constexpr( std::is_trivial<K>() )
				{
					ReadObject<K>( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						key = ReadString( rStream );
					}
					else
					{
						K::Deserialise( key, rStream );
					}
				}

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
				if constexpr( std::is_trivial<K>() )
				{
					ReadObject<K>( key, rStream );
				}
				else
				{
					if constexpr( std::is_same<K, std::string>() )
					{
						key = ReadString( rStream );
					}
					else
					{
						K::Deserialise( key, rStream );
					}
				}

				V value{};
				if constexpr( std::is_trivial<V>() )
				{
					ReadObject<V>( value, rStream );
				}
				else
				{
					if constexpr( std::is_same<V, std::string>() )
					{
						value = ReadString( rStream );
					}
					else
					{
						V::Deserialise( value, rStream );
					}
				}

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
		// MATH & SATURN BUFFERS

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
	};
}