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
#include "LocalisationTableCsvHelper.h"

#include "LocalisationTableAsset.h"

namespace Saturn {

	void LocalisationTableCsvHelper::ExportCSV( Ref<class LocalisationTableAsset> tbl, const std::filesystem::path& rWhere )
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

	static std::vector<std::string> SplitString( const std::string& rString ) 
	{
		std::vector<std::string> strs;

		std::string workingCopy = rString;
		std::string del = ",";

		size_t pos = 0llu;
		while( ( pos = workingCopy.find( del ) ) != std::string::npos )
		{
			strs.push_back( workingCopy.substr( 0, pos ) );
			workingCopy.erase( 0, pos + del.length() );
		}

		strs.push_back( workingCopy );
		return strs;
	}

	bool LocalisationTableCsvHelper::ParseHeaderLine( const std::string& rLine, Ref<LocalisationTableAsset> tbl )
	{
		// This is what the header row looks like:
		// EntryName,EntryType,[langName1 (langNumber1)],[langName2 (langNumber2)],...

		size_t index = 0llu;
		for( const auto& rSection : SplitString( rLine ) )
		{
			if( index < 2 )
			{
				++index;
				continue;
			}

			const auto startingBracket = rSection.find_first_of( '(' );
			const auto endingBracket = rSection.find_first_of( ')' );

			if( endingBracket < startingBracket )
				return false;

			if( ( endingBracket - startingBracket ) == 1 )
				return false;

			const auto langNumberStr = rSection.substr( startingBracket + 1, endingBracket - startingBracket - 1 );
			const auto langNumber = std::stoi( langNumberStr );

			const LocTextLanguage langauge = ( LocTextLanguage ) langNumber;
			if( langauge > LocTextLanguage::Zulu )
				return false;

			tbl->AddNewLanguage( langauge, LocTextLanguageVariant::UnknownUnknown );

			++index;
		}

		return true;
	}

	bool LocalisationTableCsvHelper::ParseEntryLine( const std::string& rLine, Ref<LocalisationTableAsset> tbl )
	{
		// A data entry line may look like:
		// LOCTEXT_MENU, 0, Settings, Ajustes
		// Name,	    Type, English, Spanish
		// -------
		const auto& rSections = SplitString( rLine );

		const auto entryTypeNum = std::stoi( rSections[ 1 ] );
		const LocalisationTableEntryType entryType = ( LocalisationTableEntryType ) entryTypeNum;
		if( entryType > LocalisationTableEntryType::MaxValue )
			return false;

		LocalisationTableEntry& rEntry = tbl->AddNewEntry();
		rEntry.Type = entryType;
		rEntry.EntryName = rSections[ 0 ];

		size_t langIndex = 2llu;
		for( const auto lang : tbl->GetUsedLanguages() )
		{
			const auto& rData = rSections[ langIndex++ ];

			auto& rEntryData = rEntry.DataPerLanguage[ lang ];
			switch( entryType )
			{
				case LocalisationTableEntryType::Asset:
				{
					const auto assetID = std::stoi( rData );
					rEntryData.SetAssetID( assetID );
				} break;

				case LocalisationTableEntryType::Text:
				{
					rEntryData.SetTextData( rData );
				} break;

				default:
					return false;
			}
		}

		return true;
	}

	bool LocalisationTableCsvHelper::ImportCSV( Ref<class LocalisationTableAsset> tbl, const std::filesystem::path& rWhere )
	{
		if( !std::filesystem::exists( rWhere ) )
			return false;

		std::ifstream stream( rWhere, std::ios::in );

		size_t lineNumber = 0llu;
		std::string line;
		while( std::getline( stream, line ) )
		{
			if( lineNumber == 0 )
			{
				ParseHeaderLine( line, tbl );
			}
			else
			{
				ParseEntryLine( line, tbl );
			}

			++lineNumber;
		}

		stream.close();
	
		return true;
	}

}
