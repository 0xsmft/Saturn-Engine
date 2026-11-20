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
#include "LocalisationTableUndoRedoActions.h"

namespace Saturn {
		
	using UnderlyingType = std::underlying_type_t< LocalisationTableEntryType >;

	UndoRedoLocalisationTableChangeEntryType::UndoRedoLocalisationTableChangeEntryType( Ref<LocalisationTableAsset> lt, LocalisationTableEntry& rEntry )
		: UndoRedoActionBase( "Modify Localisation Table Entry Change" ), m_LocalisationTableAsset( lt ), m_rEntry( rEntry )
	{
		m_OldType = m_rEntry.Type;
		m_CopyOfDataTablePerType[ ( UnderlyingType )m_OldType ] = m_rEntry.DataPerLanguage;
	}

	UndoRedoLocalisationTableChangeEntryType::~UndoRedoLocalisationTableChangeEntryType()
	{
	}

	void UndoRedoLocalisationTableChangeEntryType::Undo()
	{
		// Before we undo, we need to save the current data.
		m_CopyOfDataTablePerType[ ( UnderlyingType ) m_rEntry.Type ] = m_rEntry.DataPerLanguage;

		const auto oldType = m_rEntry.Type;
		m_LocalisationTableAsset->ChangeTypeForEntry_UndoRedo( m_rEntry, m_OldType, m_CopyOfDataTablePerType[ ( UnderlyingType ) m_OldType ] );
		m_OldType = oldType;
	}

	void UndoRedoLocalisationTableChangeEntryType::Redo()
	{
		// Before we undo, we need to save the current data.
		m_CopyOfDataTablePerType[ ( UnderlyingType ) m_rEntry.Type ] = m_rEntry.DataPerLanguage;

		const auto oldType = m_rEntry.Type;
		m_LocalisationTableAsset->ChangeTypeForEntry_UndoRedo( m_rEntry, m_OldType, m_CopyOfDataTablePerType[ ( UnderlyingType ) m_OldType ] );
		m_OldType = oldType;
	}

}
