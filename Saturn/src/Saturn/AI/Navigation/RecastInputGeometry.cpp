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
#include "RecastInputGeometry.h"

#include <Recast/RecastChunkyTriMesh.h>

#include "Saturn/AI/Navigation/NavBoundsEntity.h"

#include "Saturn/Scene/Scene.h"
#include "Saturn/Physics/PhysicsScene.h"

#include <Recast/Recast.h>

namespace Saturn {

	void RecastInputGeometry::BeginImport()
	{
		if( m_pChunkyTriMesh )
		{
			delete m_pChunkyTriMesh;
			m_pChunkyTriMesh = nullptr;
		}

		m_pChunkyTriMesh = new rcChunkyTriMesh();
	}

	void RecastInputGeometry::EndImport( const AABB& rAABB )
	{
		m_MinBounds = rAABB.Min;
		m_MaxBounds = rAABB.Max;

		rcCreateChunkyTriMesh( m_ExportData.VertexBuffer.data(), m_ExportData.IndexBuffer.data(), ( int ) m_ExportData.IndexBuffer.size() / 3, 256, m_pChunkyTriMesh );
	}

	//////////////////////////////////////////////////////////////////////////
	// PhysicsSceneExporter

	void PhysicsSceneExporter::Export( RecastInputGeometry& rInputGeom, AABB& rNavMeshBounds )
	{
		g_ActiveScene->GetPhysicsScene()->ExportRc( rInputGeom.GetExportData(), rNavMeshBounds );
	}

}
