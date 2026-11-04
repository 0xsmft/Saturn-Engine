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

#pragma once

#include "Saturn/Asset/Asset.h"

#include "Saturn/LocText/Language.h"

namespace Saturn {

	enum class LocalisationTableEntryType : uint8_t
	{
		Text,
		Asset,

		MaxValue = Asset
	};

	constexpr std::string_view LocalisationTableEntryTypeToString( LocalisationTableEntryType type )
	{
		switch( type )
		{
			case LocalisationTableEntryType::Text:
				return "Text";
		
			case LocalisationTableEntryType::Asset:
				return "Asset";
			
			default:
				break;
		}

		SAT_CORE_ASSERT( false );
		return "Unknown";
	}

	//
	// Data for one entry and one language.
	//
	class LocalisationTableEntryData
	{
	public:
		using StringType = std::string;

	public:
		LocalisationTableEntryType Type = LocalisationTableEntryType::Text;

	public:
		LocalisationTableEntryData( LocalisationTableEntryType type = LocalisationTableEntryType::Text )
			: Type( type )
		{
			switch( Type )
			{
				default: break;
				case LocalisationTableEntryType::Asset:
				{
					SetAssetID( 0 );
				} break;
			}
		}
		~LocalisationTableEntryData() = default;

		const StringType& GetTextData() const
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Text );
			return std::get<StringType>( Data );
		}

		StringType& GetTextData()
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Text );
			return std::get<StringType>( Data );
		}

		AssetID GetAssetID() const
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Asset );
			return std::get<AssetID>( Data );
		}

		inline void SetAssetID( AssetID id ) 
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Asset );
			Data = id;
		}

		inline void SetTextData( const StringType& rNewString )
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Text );
			Data = rNewString;
		}

	private:
		std::variant<StringType, AssetID> Data;
	};

	//
	// LocalisationTableEntry
	//
	// Data for one entry and all of it's language.
	//
	struct LocalisationTableEntry
	{
		LocalisationTableEntry() = default;

		LocalisationTableEntry( const std::string& rEntryName, LocalisationTableEntryType type = LocalisationTableEntryType::Text )
			: EntryName( rEntryName ), Type( type )
		{
		}

		std::string EntryName;

		//					LANGUAGE      ->			ENTRIES
		std::unordered_map<LocTextLanguage, LocalisationTableEntryData>	DataPerLanguage;
	
		// What type of entry is this? By default it's a text entry.
		LocalisationTableEntryType Type = LocalisationTableEntryType::Text;
	};

	//
	// LocalisationTableAsset
	//
	// A localisation table is a list of all entires (keys) that are mapped to a
	// piece of data, that can be text data (for use in UI) or an AssetID.
	//
	// Languages have to be added and exist throughout the table.
	// For example adding Spanish to the table creates a new data field in all
	// entires. Removing a language has the same effect.
	//
	// LocalisationTables are only used for UI, so in the AluraCanvas you can set the
	// primary table to use.
	//
	class LocalisationTableAsset : public Asset
	{
	public:
		// We have to use a UUID because the name of an entry is volatile
		// and can change, but of course a map needs a constant key value.
		//
		//
		//											UUID -> ENTRY
		using LocTableEntryMap = std::unordered_map<UUID, LocalisationTableEntry>;

		// Runtime (built, non-volatile) version of the regular entry map.
		//
		// We have a separate map because during runtime we'll be accessing this data from it's
		// entry name not it's ID. So having a dedicated map allows for better performance.
		//
		// We store a pointer to the entry (that is stored in the volatile map) because there is no
		// need to copy here.
		// 
		// This map is built when we call BuildIfNeeded().
		// 
		//											ENTRY NAME ->  ENTRY*
		using LocTableEntryMapRT = std::unordered_map<std::string, const LocalisationTableEntry*>;

	public:
		LocalisationTableAsset();
		LocalisationTableAsset( const Ref<Asset>& rBase );
		virtual ~LocalisationTableAsset();

		void BuildIfNeeded();
	
		std::string RetrieveText( const std::string& rTableEntry ) const;

	public:
		LocTableEntryMap& GetAllEntries() { return m_Entries; }
		const LocTableEntryMap& GetAllEntries() const { return m_Entries; }

		std::vector<LocTextLanguage>& GetUsedLanguages() { return m_UsedLanguages; }
		const std::vector<LocTextLanguage>& GetUsedLanguages() const { return m_UsedLanguages; }

		LocTextLanguage GetActiveLanguage() const { return m_ActiveLanguage; }

		[[nodiscard]] bool IsLanguageAlreadyUsed( LocTextLanguage lang ) const;

	private:
		uint64_t GetNewNameCount();
		LocalisationTableEntry& AddNewEntry();
		void RemoveEntry( UUID id );
		void RemoveLanguage( LocTextLanguage language );

	private:
		void AddNewLanguage( LocTextLanguage lang, LocTextLanguageVariant dialect );
		void AddNewLanguageToAllEntires( LocTextLanguage lang, LocTextLanguageVariant dialect );
		void AppendLanguagesToNewlyAddedEntry( LocalisationTableEntry& rEntry );

		//
		// Change the data type from one to another.
		// 
		// NB: All data will be ERASED.
		//
		void ChangeTypeForEntry( LocalisationTableEntry& rEntry, LocalisationTableEntryType newType );

	private:
		LocTableEntryMap m_Entries;
		LocTableEntryMapRT m_EntriesRT;
		std::vector<LocTextLanguage> m_UsedLanguages;
		LocTextLanguage m_ActiveLanguage = LocTextLanguage::Unknown;

	private:
		friend class LocalisationTableAssetViewer;
		friend class LocalisationTableAssetSerialiser;
	};

}
