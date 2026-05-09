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

#include "Saturn/Core/Base.h"
#include "Saturn/NodeEditor/NodeEditorBase.h"

namespace Saturn {

	class NodeCacheSettings
	{
	public:
		static bool WriteEditorSettings( SharedPtr<NodeEditor> rNodeEditor );
		static void ReadEditorSettings( NodeEditor* pNodeEditor );

	private:
		static bool CanAppendFile( const std::filesystem::path& rFilepath );

		static void OverrideFile( const std::filesystem::path& rFilepath, SharedPtr<NodeEditor> rNodeEditor );
		static void AppendFile( const std::filesystem::path& rFilepath, SharedPtr<NodeEditor> rNodeEditor );
	};

	class NodeCacheEditor
	{
	public:
		static void WriteNodeEditorCache( SharedPtr<NodeEditor> nodeEditor, const std::string& rCustomName = "" );
		static bool ReadNodeEditorCache( SharedPtr<NodeEditor> nodeEditor, AssetID id, const std::string& rCustomName = "" );

		static bool ReadNodeTaskCacheOnly( NodeTaskCache& rNodeTaskCache, AssetID id, const std::string& rCustomName = "" );

		// Dist
		static void ConvertToDistNC( AssetID id, const std::string& rCustomName = "" );
	};
	
}