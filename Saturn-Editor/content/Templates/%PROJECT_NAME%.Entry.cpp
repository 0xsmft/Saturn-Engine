/********************************************************************************************
*                                                                                           *
*                                                                                           *
*                                                                                           *
* MIT License                                                                               *
*                                                                                           *
* Copyright (c) 2023 BEAST                                                                  *
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

// Game client main.
/* Generated code, DO NOT modify! */
// This files supports Saturn version 0.1.4 (4100)

#if defined(_WIN32)
#include <Windows.h>
#endif

#include <Saturn/Core/App.h>
#include <Saturn/Core/ErrorDialog.h>
#include <Saturn/Runtime/RuntimeLayer.h>
#include <Saturn/Project/Project.h>
#include <Saturn/Vulkan/ShaderBundle.h>
#include <Saturn/Core/Timer.h>
#include <Saturn/Vulkan/Renderer2D.h>
#include <Saturn/Vulkan/SceneRenderer.h>
#include <Saturn/Serialisation/AssetBundle.h>

// Saturn client main:
extern int _main( int, char** );

#if !defined(SAT_DIST) || defined( __X31_SHOWCONSOLE__ )
int main( int count, char** args )
{
	// Hand it off to Saturn:
	return _main( count, args );
}
#endif

#if defined ( _WIN32 )

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
	return _main( __argc, __argv );
}

#endif // _WIN32

// Client default app.
class __GameApplication final : public Saturn::Application
{
public:
	explicit __GameApplication( const Saturn::ApplicationSpecification& spec )
		: Saturn::Application( spec )
	{
		std::filesystem::path workingDir = std::filesystem::current_path();

		if( const auto result = Saturn::AssetBundle::ReadMinimal(); result != Saturn::AssetBundleResult::Success ) 
		{
			std::string errorMessage = std::format( "Failed to load AB-Minimal bundle! Error Code: {0}", ( int ) result );
			SAT_CORE_VERIFY( false, errorMessage );
		}

		RootContentPath = workingDir / "content";

		// Load the shader bundle.
		if( const auto result = Saturn::ShaderBundle::ReadBundle(); result != Saturn::ShaderBundleResult::Success )
		{
			std::string errorMessage = std::format( "Failed to load shader bundle! Error Code: {0}", ( int ) result );
			SAT_CORE_VERIFY( false, errorMessage );
		}

		Saturn::Renderer2D::Get().Init();

		Saturn::SceneRendererFlags flags = Saturn::SceneRendererFlag_MasterInstance | Saturn::SceneRendererFlag_SwapchainTarget;
		m_SceneRenderer = new Saturn::SceneRenderer( flags );
	}

	virtual void OnInit() override
	{
		m_RuntimeLayer = new Saturn::RuntimeLayer();
		PushLayer( m_RuntimeLayer );
	}

	virtual void OnShutdown() override
	{
		PopLayer( m_RuntimeLayer );
		delete m_RuntimeLayer;
	}

private:
	Saturn::RuntimeLayer* m_RuntimeLayer = nullptr;
};

Saturn::Application* Saturn::CreateApplication( int argc, char** argv )
{
	std::filesystem::path WorkingDir = argv[ 0 ];
	std::filesystem::current_path( WorkingDir.parent_path() );

	Saturn::ApplicationSpecification spec{};
	spec.Flags = Saturn::ApplicationFlag_CreateSceneRenderer | Saturn::ApplicationFlag_UseVFS | Saturn::ApplicationFlag_UseGameThread;

	return new __GameApplication( spec );
}