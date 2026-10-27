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

#pragma once

#include "Saturn/Asset/Asset.h"

#include "Saturn/LocText/Language.h"

namespace Saturn {

	enum class LocalisationTableEntryType : uint8_t
	{
		Text,
		Asset
	};

	class LocalisationTableEntryData
	{
	public:
		LocalisationTableEntryType Type = LocalisationTableEntryType::Text;

	public:
		LocalisationTableEntryData() = default;
		~LocalisationTableEntryData() = default;

		const std::u8string& GetTextData() const 
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Text );
			return std::get<std::u8string>( Data ); 
		}

		std::u8string& GetTextData()
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Text );
			return std::get<std::u8string>( Data );
		}

		AssetID GetAssetID() const
		{
			SAT_CORE_ASSERT( Type == LocalisationTableEntryType::Asset );
			return std::get<AssetID>( Data );
		}

	private:
		std::variant<std::u8string, AssetID> Data;
	};

	struct LocalisationTableEntry
	{
		LocalisationTableEntry() = default;

		LocalisationTableEntry( const std::string& rEntryName )
			: EntryName( rEntryName )
		{
		}

		std::string EntryName;
		std::unordered_map<LocTextLanguage, LocalisationTableEntryData>	DataPerLanguage;
	};

	class LocalisationTableAsset : public Asset
	{
	public:
		//											UUID -> ENTRY
		using LocTableEntryMap = std::unordered_map<UUID, LocalisationTableEntry>;

	public:
		LocalisationTableAsset();
		LocalisationTableAsset( const Ref<Asset>& rBase );
		virtual ~LocalisationTableAsset();

	public:
		LocTableEntryMap& GetAllEntries() { return m_Entries; }
		const LocTableEntryMap& GetAllEntries() const { return m_Entries; }

		std::vector<LocTextLanguage>& GetUsedLanguages() { return m_UsedLanguages; }
		const std::vector<LocTextLanguage>& GetUsedLanguages() const { return m_UsedLanguages; }

	private:
		uint64_t GetNewNameCount();
		LocalisationTableEntry& AddNewEntry();

	private:
		LocTableEntryMap m_Entries;
		std::vector<LocTextLanguage> m_UsedLanguages;

	private:
		friend class LocalisationTableAssetViewer;
		friend class LocalisationTableAssetSerialiser;
	};

}
