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

#include <map>
#include <string>

namespace Saturn {

	enum class HeaderToolError : int
	{
		/* Code Generation Errors */
		CG001,
		CG002,
		CG003,
		CG004,

		/* Header tool errors  */
		TR000, /* internal error */
		TR000A,
		TR001,
		TR002,
		TR003,
		TR004,
	};

	enum class HeaderToolWarning
	{
		CG001H,
		CG002A
	};

	static std::map<HeaderToolError, std::string> s_ErrorsMaps 
	{
		{ HeaderToolError::CG001,  ":error (CG001) | No SCLASS macro was found in header file! Valid usage may follow: SCLASS(<args>)" },
		{ HeaderToolError::CG002,  ":error (CG002) | GENERATED_BODY macro was found in header file! Valid usage may follow: GENERATED_BODY()" },
		{ HeaderToolError::CG003,  ":error (CG003) | Expected variable definition after SPROPERTY macro." },
		{ HeaderToolError::CG004,  ":error (CG004) | GENERATED_BODY/SCLASS was used, however no base class was specified. You must specifiy a base class driving from Saturn::SObject" },

		// Header tool errors must have "ERROR:" so that VS picks it up as an error can will actually display it in error list, using ":error" sometimes works, however "ERROR:" will flag it as an execution error.
		{ HeaderToolError::TR000,  "ERROR: (TR000) | Internal Error." },
		{ HeaderToolError::TR000A, "ERROR: (TR000A) | Code generation terminated." },
		{ HeaderToolError::TR001,  "ERROR: (TR001) | Missing /SRC Argument. Valid usage is /SRC=<path_to_src>" },
		{ HeaderToolError::TR002,  "ERROR: (TR002) | Missing /OUT Argument. Valid usage is /OUT=<path_to_output>" },
		{ HeaderToolError::TR003,  "ERROR: (TR003) | Missing /FC Argument. Valid usage is /FC=<path_to_filecache>." },
		{ HeaderToolError::TR004,  "ERROR: (TR003) | Missing configuration argument. Valid usage is /DEBUG or /RELEASE or /DIST" },
	};

	static std::map<HeaderToolWarning, std::string> s_WarningMaps
	{
		{ HeaderToolWarning::CG001H,  ":warning (CG001H) | Unknown SCLASS argument! Argument omitted." },
		{ HeaderToolWarning::CG002A,  ":warning (CG002A) | No arguments are allowed in the GENERATED_BODY macro, arguments omitted." },
	};

}
