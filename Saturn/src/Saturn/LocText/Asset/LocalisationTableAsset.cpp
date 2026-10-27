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
#include "LocalisationTableAsset.h"

namespace Saturn {

	LocalisationTableAsset::LocalisationTableAsset()
	{
	}

	LocalisationTableAsset::LocalisationTableAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

	LocalisationTableAsset::~LocalisationTableAsset()
	{

	}

	uint64_t LocalisationTableAsset::GetNewNameCount()
	{
		uint64_t count = ( uint64_t ) std::count_if( m_Entries.begin(), m_Entries.end(),
			[]( const auto& rKV ) -> bool
		{
			// This is not great...
			// A name like "My New Text Entry" would pass this check
			// but im too lazy to write the correct way so this will do.
			if( rKV.second.EntryName.contains( "New Text Entry" ) )
				return true;
			else
				return false;
		} );

		return count;
	}

	LocalisationTableEntry& LocalisationTableAsset::AddNewEntry()
	{
		const auto count = GetNewNameCount();

		auto emplaced = m_Entries.emplace( std::pair{ UUID(), LocalisationTableEntry( count == 0 ? "New Text Entry" : std::format( "New Text Entry ({})", count ) ) } );

		auto& rKv = *( emplaced.first );
		return rKv.second;
	}

}
