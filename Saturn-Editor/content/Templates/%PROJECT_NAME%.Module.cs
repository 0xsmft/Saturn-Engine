using System;
using System.IO;

using SaturnBuildTool;

public class %PROJECT_NAME%Module : Module
{
    public override void Init()
    {
        base.Init();

        Name = "%PROJECT_NAME%";
        OutputType = LinkerOutput.SharedLibrary;
        
        // Include directories relative to root folder (solution directory)
        string saturnDir = Shared.ProjectInfo.SaturnDir;
        Includes.AddRange( new string[] {
            "Source/%PROJECT_NAME%",
            "Build/Generated",
            Path.Combine( saturnDir, "Saturn/src" ),
            Path.Combine( saturnDir, "Saturn-SharedStorage/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/imgui" ),
            Path.Combine( saturnDir, "Saturn/vendor/glm" ),
            Path.Combine( saturnDir, "Saturn/vendor/entt/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/assimp/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/shaderc/libshaderc/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/shaderc/glslc/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/SPIRV-Cross/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/vma/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/ImGuizmo/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/yaml-cpp/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/imgui_node_editor" ),
            Path.Combine( saturnDir, "Saturn/vendor/imspinner/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/tracy/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/Filewatch/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/miniaudio/src" ),
            Path.Combine( saturnDir, "Saturn/vendor/physx/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/zlib" ),
            Path.Combine( saturnDir, "Saturn/vendor/Recast/RecastAndDetour/Include" ),
            Path.Combine( saturnDir, "Saturn/vendor/stb" ),
            Path.Combine( saturnDir, "Saturn/vendor/vulkan/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/spdlog/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/physx/include/pxshared" ),
            Path.Combine( saturnDir, "Saturn/vendor/physx/include/physx" ),
            Path.Combine( saturnDir, "Saturn/vendor/acl/include" ),
            Path.Combine( saturnDir, "Saturn/vendor/acl/rtm/include" ),
        } );

        string binPath = Path.Combine( saturnDir, "bin" );
        binPath = Path.Combine( binPath, VendorBinaries.GetRootBinPath( binPath ) );

        LibraryPaths.AddRange( new string[] {
            Path.Combine( binPath, "Saturn" ),
            VendorBinaries.GetBinPath( VendorProject.PHYSX ),
            VendorBinaries.GetBinPath( VendorProject.YAML_CPP ),
            VendorBinaries.GetBinPath( VendorProject.IMGUI ),
            VendorBinaries.GetBinPath( VendorProject.SPIRVCROSS ),
            VendorBinaries.GetBinPath( VendorProject.SHADERC ),
            VendorBinaries.GetBinPath( VendorProject.TRACY ),
            VendorBinaries.GetBinPath( VendorProject.ZLIB ),
            VendorBinaries.GetBinPath( VendorProject.RECAST ),
            VendorBinaries.GetBinPath( VendorProject.FREETYPE ),
            VendorBinaries.GetBinPath( VendorProject.MSDF ),
            VendorBinaries.GetBinPath( VendorProject.MSDFGEN ),
        } );

        binPath = Path.Combine( binPath, "Saturn-SharedStorage" );

        LibraryPaths.Add( binPath );

        PCH = new PCHInfo( null, null );

        // NOTE: When using UseTargetDirectory we don't have to specify a SourcePath
        SourceDirectoryOptions = SourceDirectoryOptions.Custom;
        OutputDirectoryOptions = OutputDirectoryOptions.UseTargetDirectory;

        SourcePaths.AddRange( new string[] {
            "Source/%PROJECT_NAME%",
        } );

        if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Debug )
        {
            Links.AddRange( new string[] {
                Path.Combine( saturnDir, "Saturn/vendor/assimp/bin/Debug/assimp-vc143-mtd.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Debug-Windows/shaderc.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Debug-Windows/shaderc_util.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Debug-Windows/glslangd.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Debug-Windows/SPIRV-Tools.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Debug-Windows/glslc.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/LowLevel_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/LowLevelAABB_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/LowLevelDynamics_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysX_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXCharacterKinematic_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXCommon_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXCooking_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXExtensions_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXFoundation_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXPvdSDK_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXTask_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/PhysXVehicle_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/SceneQuery_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Debug/SimulationController_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/vulkan/bin/vulkan-1.lib" ),
            } );
        }
        
        if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Release ) 
        {
            Links.AddRange( new string[] {
                Path.Combine( saturnDir, "Saturn/vendor/assimp/bin/Release/assimp-vc143-mt.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Release-Windows/shaderc.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Release-Windows/shaderc_util.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Release-Windows/glslang.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Release-Windows/SPIRV-Tools.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/shaderc/bin/Release-Windows/glslc.lib" ),
            } );
        }
        
        if( Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Release || Shared.ProjectInfo.CurrentConfigKind == ConfigKind.Dist )
        {
            Links.AddRange( new string[] {
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/LowLevel_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/LowLevelAABB_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/LowLevelDynamics_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysX_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXCharacterKinematic_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXCommon_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXCooking_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXExtensions_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXFoundation_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXPvdSDK_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXTask_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/PhysXVehicle_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/SceneQuery_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/physx/bin/Release/SimulationController_static_64.lib" ),
                Path.Combine( saturnDir, "Saturn/vendor/vulkan/bin/vulkan-1.lib" )
            } );
        }
    }
}
