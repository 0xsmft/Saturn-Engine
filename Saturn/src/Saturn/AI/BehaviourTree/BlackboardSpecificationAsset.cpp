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
#include "BlackboardSpecificationAsset.h"

#include "Blackboard.h"

#if !defined( SAT_DIST )
#include "Saturn/ImGui/ImGuiAuxiliary.h"
#endif

namespace Saturn {

	BlackboardSpecificationAsset::BlackboardSpecificationAsset( const Ref<Asset>& rBase )
		: Asset( rBase )
	{
	}

#if !defined( SAT_DIST )
	Ref<BlackboardVaraibleSpec> BlackboardSpecificationAsset::DrawKeyFinder( NodeEditorVariableDataType type, Ref<BlackboardVaraibleSpec> selectedVar )
	{
		for( auto& rVariable : m_SpecificationData )
		{
			if( type != NodeEditorVariableDataType::Unknown && rVariable->DataType != type )
				continue;

			bool isSelected = selectedVar->VariableID == rVariable->VariableID;
			if( ImGui::Selectable( rVariable->Name.c_str(), isSelected ) )
			{
				rVariable->IsActive = true;
				selectedVar->IsActive = false;

				return rVariable;
			}
		}

		return nullptr;
	}

#endif

	Ref<BlackboardVaraibleSpec> BlackboardSpecificationAsset::PostInitKey( UUID variableID )
	{
		const auto itr = std::find_if( m_SpecificationData.begin(), m_SpecificationData.end(),
			[ variableID ]( const auto& rItem )
		{
			return rItem->VariableID == variableID;
		} );

		if( itr != m_SpecificationData.end() )
		{
#if !defined(SAT_DIST)
			auto& var = *itr;
			var->IsActive = true;

			return var;
#else
			return *itr;
#endif
		}

		return nullptr;
	}

	Ref<BlackboardVaraibleSpec> BlackboardSpecificationAsset::GetKeySpec( UUID variableID ) const
	{
		const auto itr = std::find_if( m_SpecificationData.begin(), m_SpecificationData.end(),
			[ variableID ]( const auto& rItem )
		{
			return rItem->VariableID == variableID;
		} );

		return itr == m_SpecificationData.end() ? nullptr : *itr;
	}

	Ref<Blackboard> BlackboardSpecificationAsset::CreateBlackboard()
	{
		Ref<Blackboard> bb = Ref<Blackboard>::Create();
		bb->InitialiseVariables( ID );
		return bb;
	}

}
