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

#include "Saturn/GameFramework/Core/ClassMetadataHandler.h"
#if !defined(SAT_DIST)
#include "Saturn/NodeEditor/NodeEditorBase.h"
#endif
#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"

#include "M2_BooleanAlgebraTasks.h"

#include "Saturn/Serialisation/Raw/RawSerialisation.h"

namespace Saturn {

	template<typename Ty>
	void Maths2GeneralBooleanTask<Ty>::Serialise( std::ofstream& rStream ) const
	{
		NodeEditorTaskBase::Serialise( rStream );

		RawSerialisation::WriteObjectChecked( m_PinA, rStream );
		RawSerialisation::WriteObjectChecked( m_PinB, rStream );
	}

	template<typename Ty>
	void Maths2GeneralBooleanTask<Ty>::Deserialise( FDependentIStream& rStream )
	{
		NodeEditorTaskBase::Deserialise( rStream );

		RawSerialisation::ReadObjectChecked( m_PinA, rStream );
		RawSerialisation::ReadObjectChecked( m_PinB, rStream );
	}
}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

SAT_X31_CREATE_AUTO_REG( Maths2LessThanFloatTask );
SAT_X31_CREATE_AUTO_REG( Maths2LessThanUIntTask );
SAT_X31_CREATE_AUTO_REG( Maths2LessThanIntTask );

SAT_X31_CREATE_AUTO_REG( Maths2LessThanOrEquFloatTask );
SAT_X31_CREATE_AUTO_REG( Maths2LessThanOrEquUIntTask );
SAT_X31_CREATE_AUTO_REG( Maths2LessThanOrEquIntTask );

SAT_X31_CREATE_AUTO_REG( Maths2GreaterThanFloatTask );
SAT_X31_CREATE_AUTO_REG( Maths2GreaterThanUIntTask );
SAT_X31_CREATE_AUTO_REG( Maths2GreaterThanIntTask );

SAT_X31_CREATE_AUTO_REG( Maths2GreaterThanOrEquFloatTask );
SAT_X31_CREATE_AUTO_REG( Maths2GreaterThanOrEquUIntTask );
SAT_X31_CREATE_AUTO_REG( Maths2GreaterThanOrEquIntTask );

SAT_X31_CREATE_AUTO_REG( Maths2EquToBoolTask );
SAT_X31_CREATE_AUTO_REG( Maths2EquToFloatTask );
SAT_X31_CREATE_AUTO_REG( Maths2EquToUIntTask );
SAT_X31_CREATE_AUTO_REG( Maths2EquToIntTask );

SAT_X31_CREATE_AUTO_REG( Maths2NotEqualToBoolTask );
SAT_X31_CREATE_AUTO_REG( Maths2NotEqualToFloatTask );
SAT_X31_CREATE_AUTO_REG( Maths2NotEqualToUIntTask );
SAT_X31_CREATE_AUTO_REG( Maths2NotEqualToIntTask );
