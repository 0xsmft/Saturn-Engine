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

#pragma once

#include "Saturn/Core/Ref.h"
#include "Saturn/Core/Timestep.h"

#include "SProperty.h"

#include <filesystem>

namespace Saturn {

	//////////////////////////////////////////////////////////////////////////
	// SClass Metadata
	// Describes the metadata of a class in the game framework.
	struct SClassMetadata 
	{
		std::string Name;
		std::string ParentClassName;

		std::filesystem::path GeneratedSourcePath;
		std::filesystem::path HeaderPath;

		// Does this class come from an external source e.g. the game.
		bool ExternalData = false;
	};

	enum class SClassFlags
	{
		None = 0,
		Spawnable = 1 << 0,
		VisibleInEditor = 1 << 1,
		NoMetadata = 1 << 2
	};

	class SClass : public RefTarget
	{
	public:
		SClass() {}
		virtual ~SClass() = default;

		virtual void BeginPlay() {}
		virtual void OnUpdate( Saturn::Timestep ts ) {}
		virtual void OnPhysicsUpdate( Saturn::Timestep ts ) {}

	private:
		SClassFlags m_Flags = SClassFlags::None;
	};

}