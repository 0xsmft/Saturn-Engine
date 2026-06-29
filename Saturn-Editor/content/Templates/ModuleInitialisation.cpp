/********************************************************************************************
*                                                                                           *
* Copyright (c) 2023 BEAST                                                           		*
*                                                                                           *
*********************************************************************************************
*/

/* Generated code, DO NOT modify! */

#if !defined(SAT_DIST)

#include <sppch.h>
#include <Saturn/Project/Project.h>
#include <Saturn/Core/Profiler.h>

extern "C" {

	SAT_DLLEXPORT void InitializeModule( Saturn::Project* pProject, const void* pTracyData )
	{
		Saturn::Project::SetActiveProject( pProject );
		tracy::InitializeModule( pTracyData );
	}
};

#endif
