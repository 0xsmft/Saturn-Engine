/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2024 BEAST                                                           *
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

#include <unordered_map>

namespace Saturn {

	class VFile;

	class VDirectory : public RefTarget
	{
	public:
		VDirectory() = default;
		VDirectory( const std::string& rName );
		VDirectory( const std::wstring& rName );
		VDirectory( const std::string& rName, VDirectory* parentDirectory );

		~VDirectory();

	public:
		void RemoveFile( const std::string& rName );
		
		void AddDirectory( const std::string& rName );
		void RemoveDirectory( const std::string& rName );

		template<typename Func>
		void EachFile( Func Function )
		{
			for( const auto& rFile : Files )
			{
				Function( rFile );
			}
		}

		template<typename Func>
		void EachDirectory( Func Function )
		{
			for( const auto& rDir : Directories )
			{
				Function( rDir );
			}
		}

		void Clear();

		const std::string& GetName()       { return m_Name; }
		const std::string& GetName() const { return m_Name; }

		VDirectory& GetParent() { return *ParentDirectory; }

	public:
		static void Serialise( const Ref<VDirectory>& rObject, std::ofstream& rStream );
		static void Deserialise( Ref<VDirectory>& rObject, std::ifstream& rStream );

	public:
		// Name -> File
		std::unordered_map< std::string, Ref< VFile > > Files;

		// Name -> VDirectory
		std::unordered_map< std::string, Ref< VDirectory > > Directories;

		VDirectory* ParentDirectory = nullptr;

	private:
		std::string m_Name;

	private:
		friend class VirtualFS;
	};
}