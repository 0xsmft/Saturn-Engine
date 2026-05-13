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

#include "Saturn/Core/Module.h"
#include "Saturn/Core/DynamicLinkLibrary.h"

#include "Saturn/GameFramework/SObject.h"
#include "Saturn/GameFramework/Core/GameScript.h"

namespace Saturn {

	class GameModule : public SObject
	{
		SAT_DECLARE_CLASS( GameModule, SObject );
	public:
		// NOTE: GameModule owned by parent application layer, EditorLayer if editor and RuntimeLayer when in Dist
		static GameModule* Get() { return SingletonStorage::GetSingleton<GameModule>(); }

	public:
		GameModule();
		~GameModule();

		void Reload();

		bool HasModule() const { return m_ModuleHandle; }

	public:		
		const std::filesystem::path& GetModulePath() const { return m_ModuleHandle->m_Path; }

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		const std::string& GetTimestamp() const { return m_LastTimestamp; }

		void BeginHotReload();
		void EndHotReload();
#endif

	private:
		void LoadModule( bool wasHotReloaded = false );
		void Unload();

	private:
		Ref<Module> m_ModuleHandle;

#if defined(SAT_DEBUG) || defined(SAT_RELEASE)
		std::string m_LastTimestamp;
#endif
	};

}
