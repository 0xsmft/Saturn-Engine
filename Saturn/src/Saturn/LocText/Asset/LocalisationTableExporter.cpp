/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2026 0xsmft https://0xsmft.dev                                       *
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
#include "LocalisationTableExporter.h"

#include "LocalisationTableAsset.h"

namespace Saturn {

	void LocalisationTableExporter::ExportCSV( Ref<class LocalisationTableAsset> tbl, const std::filesystem::path& rWhere )
	{
		std::ofstream fout( rWhere, std::ios::out );

		// Header row
		// --------
		// This will give us a result of:
		// EntryName,EntryType,[langName1 (langNumber1)],[langName2 (langNumber2)],...

		fout << "EntryName,EntryType,";

		// Langauges
		const auto& rUsedLangauges = tbl->GetUsedLanguages();
		for( size_t i = 0; i < tbl->GetUsedLanguages().size(); ++i )
		{
			fout << Auxiliary::LocTextLanguageToString( rUsedLangauges.at( i ) )
				<< "("
				<< ( std::underlying_type_t<LocTextLanguage> ) rUsedLangauges.at( i )
				<< ")";

			if( i + 1 < tbl->GetUsedLanguages().size() )
				fout << ",";
		}

		fout << "\n";

		// Data
		const auto& rEntries = tbl->GetAllEntries();

		for( const auto& [rID, rEntry] : rEntries )
		{
			fout << rEntry.EntryName 
				<< "," 
				<< ( uint64_t )rEntry.Type 
				<< ",";

			size_t langaugeIndex = 0llu;
			for( const auto& [rLangauge, rData] : rEntry.DataPerLanguage )
			{
				switch( rData.Type )
				{
					case LocalisationTableEntryType::Asset:
					{
						fout << rData.GetAssetID();
					} break;

					case LocalisationTableEntryType::Text:
					{
						fout << rData.GetTextData();
					} break;

					default:
						break;
				}

				if( langaugeIndex + 1 < rEntry.DataPerLanguage.size() )
					fout << ",";
				
				++langaugeIndex;
			}

			fout << "\n";
		}

		fout.close();
	}

}
