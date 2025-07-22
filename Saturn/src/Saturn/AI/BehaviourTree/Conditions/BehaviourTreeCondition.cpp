/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2020 - 2025 BEAST                                                           *
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
#include "BehaviourTreeCondition.h"

#include "Saturn/Serialisation/RawSerialisation.h"

#if !defined( SAT_DIST )
#include "Saturn/Asset/AssetManager.h"
#endif

namespace Saturn {

	void BehaviourTreeCondition::SetupMemVariable( AssetID memSpecID )
	{
#if !defined( SAT_DIST )
		m_BlackboardSpec = AssetManager::Get().GetAssetAs<BehaviourTreeMemorySpecification>( memSpecID );
#endif
	}

	//////////////////////////////////////////////////////////////////////////

	void BehaviourTreeCondition::Serialise( std::ofstream& rStream ) const
	{
		RawSerialisation::WriteObject( m_RTBlackboardVariableID, rStream );
		RawSerialisation::WriteObject( (std::underlying_type_t<BehaviourTreeConditionType>)m_ConditionType, rStream );
#if !defined(SAT_DIST)
		RawSerialisation::WriteString( m_Title, rStream );
#endif
	}

	void BehaviourTreeCondition::Deserialise( FDependentIStream& rStream )
	{
		RawSerialisation::ReadObject( m_RTBlackboardVariableID, rStream );
		RawSerialisation::ReadObject( m_ConditionType, rStream );

#if !defined(SAT_DIST)
		m_Title = RawSerialisation::ReadString( rStream );
#endif
	}

}

//////////////////////////////////////////////////////////////////////////

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( BehaviourTreeCondition );