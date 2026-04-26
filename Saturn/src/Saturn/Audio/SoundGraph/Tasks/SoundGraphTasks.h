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

#include "Saturn/NodeEditor/NodeEditorTaskBase.h"

namespace Saturn {

	SCLASS()
	class SGraphSoundOutputTask : public NodeEditorTaskBase 
	{
		SAT_DECLARE_CLASS_MOVE( SGraphSoundOutputTask, NodeEditorTaskBase );
	public:
		SGraphSoundOutputTask();
		virtual ~SGraphSoundOutputTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;
	};

	SCLASS()
	class SGraphSoundPlayerTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS_MOVE( SGraphSoundPlayerTask, NodeEditorTaskBase );
	public:
		SGraphSoundPlayerTask();
		virtual ~SGraphSoundPlayerTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		uint64_t m_SpecAssetID = 0;
		size_t m_SoundIndex = 0llu;
		bool m_Spatialisation = false;
	};

	SCLASS()
	class SGraphSoundPitchTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS_MOVE( SGraphSoundPitchTask, NodeEditorTaskBase );
	public:
		SGraphSoundPitchTask();
		virtual ~SGraphSoundPitchTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		UUID m_SoundNodeID = 0llu;
		size_t* m_pTargetSoundIndex = 0llu;
		float m_Pitch = 1.0f;
	};

	SCLASS()
	class SGraphSoundRandomPitchTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS_MOVE( SGraphSoundRandomPitchTask, NodeEditorTaskBase );
	public:
		SGraphSoundRandomPitchTask();
		virtual ~SGraphSoundRandomPitchTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		UUID m_SoundNodeID = 0llu;
		size_t* m_pTargetSoundIndex = 0llu;

		float m_MinPitch = 1.0f;
		float m_MaxPitch = 2.0f;
	};

	SCLASS()
	class SGraphSoundRandomSoundTask : public NodeEditorTaskBase
	{
		SAT_DECLARE_CLASS_MOVE( SGraphSoundRandomSoundTask, NodeEditorTaskBase );
	public:
		SGraphSoundRandomSoundTask();
		virtual ~SGraphSoundRandomSoundTask();

#if !defined(SAT_DIST)
		virtual void PreInitialiseTask( NodeEditorBase* pEditor, NodeEditorNodeBase* pNode ) override;
#endif

		virtual void InitialiseTaskWithOther( NodeEditorTaskHandler* pHandler, NodeEditorTaskBase* pOther ) override;

		virtual NodeEditorTaskState Tick( Timestep ts ) override;

	public:
		virtual void Serialise( std::ofstream& rStream ) const override;
		virtual void Deserialise( FDependentIStream& rStream ) override;

	private:
		UUID m_PinANode = 0llu, m_PinBNode = 0llu;

		size_t* m_pIndexA = nullptr;
		size_t* m_pIndexB = nullptr;
		
		size_t m_ChosenIndex = 0llu;
	};
}
