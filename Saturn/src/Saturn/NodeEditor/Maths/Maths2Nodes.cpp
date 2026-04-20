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

#include "Saturn/NodeEditor/NodeEditorBase.h"
#include "Saturn/NodeEditor/NodeEditorTaskHandler.h"

#include "Maths2Nodes.h"

namespace Saturn {

}

#include "Saturn/GameFramework/Core/EngineGenerated.h"

// I did want to avoid this....
// seems like it may be unavoidable until I get smarter...

SAT_X31_CREATE_AUTO_REG( SMaths2LessThanFloats );
SAT_X31_CREATE_AUTO_REG( SMaths2LessThanInts );
SAT_X31_CREATE_AUTO_REG( SMaths2LessThanUInts );

SAT_X31_CREATE_AUTO_REG( SMaths2LessThanOrEquFloats );
SAT_X31_CREATE_AUTO_REG( SMaths2LessThanOrEquInts );
SAT_X31_CREATE_AUTO_REG( SMaths2LessThanOrEquUInts );

SAT_X31_CREATE_AUTO_REG( SMaths2GreaterThanFloats );
SAT_X31_CREATE_AUTO_REG( SMaths2GreaterThanInts );
SAT_X31_CREATE_AUTO_REG( SMaths2GreaterThanUInts );

SAT_X31_CREATE_AUTO_REG( SMaths2GreaterThanOrEquFloats );
SAT_X31_CREATE_AUTO_REG( SMaths2GreaterThanOrEquInts );
SAT_X31_CREATE_AUTO_REG( SMaths2GreaterThanOrEquUInts );

SAT_X31_CREATE_AUTO_REG( SMaths2EqualsBool );
SAT_X31_CREATE_AUTO_REG( SMaths2EqualsFloat );
SAT_X31_CREATE_AUTO_REG( SMaths2EqualsInt );
SAT_X31_CREATE_AUTO_REG( SMaths2EqualsUInt );

SAT_X31_CREATE_AUTO_REG( SMaths2NotEqualToBool );
SAT_X31_CREATE_AUTO_REG( SMaths2NotEqualToFloat );
SAT_X31_CREATE_AUTO_REG( SMaths2NotEqualToInt );
SAT_X31_CREATE_AUTO_REG( SMaths2NotEqualToUInt );
