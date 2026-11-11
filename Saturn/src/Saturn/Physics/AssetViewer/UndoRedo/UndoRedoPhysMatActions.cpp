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
#include "UndoRedoPhysMatActions.h"

namespace Saturn {

	UndoRedoActionModifyPhysicsMaterialStaticFriction::UndoRedoActionModifyPhysicsMaterialStaticFriction( Ref<PhysicsMaterialAsset> physMat )
		: UndoRedoActionBase( "Modify Physics Material Friction" ), m_PhysicsMaterial( physMat )
	{
		SAT_CORE_ASSERT( physMat );

		m_OldFrictionValue = m_PhysicsMaterial->GetFriction();
	}

	UndoRedoActionModifyPhysicsMaterialStaticFriction::~UndoRedoActionModifyPhysicsMaterialStaticFriction()
	{
		m_OldFrictionValue = 0.0f;
	}

	void UndoRedoActionModifyPhysicsMaterialStaticFriction::Undo()
	{
		const float val = m_OldFrictionValue;
		m_OldFrictionValue = m_PhysicsMaterial->GetFriction();
		m_PhysicsMaterial->SetFriction( val );
	}

	void UndoRedoActionModifyPhysicsMaterialStaticFriction::Redo()
	{
		const float val = m_OldFrictionValue;
		m_OldFrictionValue = m_PhysicsMaterial->GetFriction();
		m_PhysicsMaterial->SetFriction( val );
	}

	//////////////////////////////////////////////////////////////////////////
	// Restitution

	UndoRedoActionModifyPhysicsMaterialRestitution::UndoRedoActionModifyPhysicsMaterialRestitution( Ref<PhysicsMaterialAsset> physMat )
		: UndoRedoActionBase( "Modify Physics Material Restitution" ), m_PhysicsMaterial( physMat )
	{
		SAT_CORE_ASSERT( physMat );
		
		m_OldRestitutionValue = m_PhysicsMaterial->GetFriction();
	}

	UndoRedoActionModifyPhysicsMaterialRestitution::~UndoRedoActionModifyPhysicsMaterialRestitution()
	{
		m_OldRestitutionValue = 0.0f;
	}

	void UndoRedoActionModifyPhysicsMaterialRestitution::Undo()
	{
		const float val = m_OldRestitutionValue;
		m_OldRestitutionValue = m_PhysicsMaterial->GetRestitution();
		m_PhysicsMaterial->SetRestitution( val );
	}

	void UndoRedoActionModifyPhysicsMaterialRestitution::Redo()
	{
		const float val = m_OldRestitutionValue;
		m_OldRestitutionValue = m_PhysicsMaterial->GetRestitution();
		m_PhysicsMaterial->SetRestitution( val );
	}

	//////////////////////////////////////////////////////////////////////////
	// Surface name

	UndoRedoActionModifyPhysicsMaterialSurfaceName::UndoRedoActionModifyPhysicsMaterialSurfaceName( Ref<PhysicsMaterialAsset> physMat )
		: UndoRedoActionBase( "Modify Physics Material Surface Name" ), m_PhysicsMaterial( physMat )
	{
		SAT_CORE_ASSERT( physMat );

		m_OldNameValue = m_PhysicsMaterial->GetSurfaceName();
	}

	UndoRedoActionModifyPhysicsMaterialSurfaceName::~UndoRedoActionModifyPhysicsMaterialSurfaceName()
	{
	}

	void UndoRedoActionModifyPhysicsMaterialSurfaceName::Undo()
	{
		const auto oldValue = m_OldNameValue;
		m_OldNameValue = m_PhysicsMaterial->GetSurfaceName();

		m_PhysicsMaterial->SetSurfaceName( oldValue );
	}

	void UndoRedoActionModifyPhysicsMaterialSurfaceName::Redo()
	{
		const auto oldValue = m_OldNameValue;
		m_OldNameValue = m_PhysicsMaterial->GetSurfaceName();

		m_PhysicsMaterial->SetSurfaceName( oldValue );
	}

}
