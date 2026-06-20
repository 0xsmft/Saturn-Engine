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

namespace Saturn {

	enum NodeEditorNodeFlags : uint8_t
	{
		// No flags
		NodeFlags_Default = 0,

		// If this flag is set the the node can not be deleted.
		NodeFlags_Irremovable = BIT( 0 ),

		// If this node has a value which is known at evaluation time/compile time then it lets the task know that there is no need to Tick.
		NodeFlags_ConstantEvaluated = BIT( 1 ),

		// Debug flag: temporary flag to let the debugging system know that this Node broke debug.
		NodeFlags_BrokeDebug = BIT( 2 ),

		// If this flag is set then the node cannot be copy pasted, useful if we have an output node.
		// because there is only meant to be output node.
		// If the node does not have this flag then it's assumed that it's safe to always allow this node
		// to be pasted again.
		NodeFlags_RejectCopyPaste = BIT( 3 ),
	};
	
}
