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
#include "Saturn/GameFramework/SProperty.h"

namespace Saturn {

	// This struct specifies a variable (key) in the Behaviour Tree Memory
	// It does not contain the actual data. It is simply a specification of what this variable should be when we convert it to a BehaviourTreeMemoryVariable struct
	class BehaviourTreeMemoryKeySpec : public RefTarget
	{
	public:
		BehaviourTreeMemoryKeySpec() = default;

		BehaviourTreeMemoryKeySpec( const std::string& rName, SPropertyType dataType, UUID varID ) 
			: Name( rName ), DataType( dataType ), VariableID( varID )
		{
		}

	public:
		// [Serialised]
		std::string Name;
		SPropertyType DataType = SPropertyType::Unknown;
		UUID VariableID = 0;

#if !defined(SAT_DIST)
		UUID RenderID;
		bool IsActive = false;
#endif
	};

	class BehaviourTreeMemorySpecification : public Asset
	{
	public:
		BehaviourTreeMemorySpecification() = default;
		BehaviourTreeMemorySpecification( const Ref<Asset>& rBase );

#if !defined( SAT_DIST )
		Ref<BehaviourTreeMemoryKeySpec> DrawKeyFinder( SPropertyType type, Ref<BehaviourTreeMemoryKeySpec> selectedVar );
#endif
		Ref<BehaviourTreeMemoryKeySpec> PostInitKey( UUID variableID );
		Ref<BehaviourTreeMemoryKeySpec> GetKeySpec( UUID variableID ) const;

		const std::vector<Ref<BehaviourTreeMemoryKeySpec>>& GetKeySpecs() const { return m_SpecificationData; }

	private:
		inline void AddNew( const std::string& rName, SPropertyType dataType, UUID varID )
		{
			m_SpecificationData.emplace_back( Ref<BehaviourTreeMemoryKeySpec>::Create( rName, dataType, varID ) );
		}

	private:
		std::vector<Ref<BehaviourTreeMemoryKeySpec>> m_SpecificationData;

	private:
		friend class BehaviourTreeMemoryAssetViewer;
		friend class BehaviourTreeMemorySpecAssetSerialiser;
		friend class RawBehaviourTreeMemorySpecSerialiser;
	};
	
}
