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

#include "Saturn/ImGui/UndoRedo/UndoRedoActionBase.h"

#include "Saturn/LocText/Asset/LocalisationTableAsset.h"

namespace Saturn {

	struct LocalisationTableEntry;

	//
	// LocalisationTableChangeEntryTypeAction
	// 
	// UndoRedo action when changing type in the asset viewer.
	//
	class UndoRedoLocalisationTableChangeEntryType : public UndoRedoActionBase
	{
	public:
		explicit UndoRedoLocalisationTableChangeEntryType( Ref<LocalisationTableAsset> lt, LocalisationTableEntry& rEntry );
		virtual ~UndoRedoLocalisationTableChangeEntryType();

	public:
		virtual void Undo() override;
		virtual void Redo() override;

	private:
		Ref<LocalisationTableAsset> m_LocalisationTableAsset = nullptr;

		std::array<LocalisationTableEntry::DataPerLanguageTable, 2u> m_CopyOfDataTablePerType{};

		LocalisationTableEntry& m_rEntry;
		LocalisationTableEntryType m_OldType = LocalisationTableEntryType::MaxValue;
	};
	
}
