newoption {
   trigger = "onlineapi",
   value = "API",
   description = "Choose a particular Online-subsystem API",
   allowed = {
      { "steam",    "Steamworks API" },
--      { "epic",  "EOS (Epic Online Services)" },
      { "none",  "No API (default)" }
   },
   default = "none"
}

workspace "Saturn"
	architecture "x64"
	startproject "Saturn-Editor"
	warnings "Default"

	configurations { "Debug", "Release", "Dist" }

	flags { "MultiProcessorCompile" }
	editandcontinue "Off"
	debugformat "c7"

	filter "action:vs*"
		linkoptions { "/ignore:4006" }

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

-- Include directories relative to root folder (solution directory)
IncludeDir = {}
IncludeDir["Ruby"] = "%{wks.location}/Saturn/vendor/Ruby/src"
IncludeDir["ImGui"] = "%{wks.location}/Saturn/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/Saturn/vendor/glm"
IncludeDir["entt"] = "%{wks.location}/Saturn/vendor/entt/include"
IncludeDir["assimp"] = "%{wks.location}/Saturn/vendor/assimp/include"
IncludeDir["shaderc"] = "%{wks.location}/Saturn/vendor/shaderc/libshaderc/include"
IncludeDir["glslc"] = "%{wks.location}/Saturn/vendor/shaderc/glslc/src"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Saturn/vendor/SPIRV-Cross/src/"
IncludeDir["vma"] = "%{wks.location}/Saturn/vendor/vma/src/"
IncludeDir["ImGuizmo"] = "%{wks.location}/Saturn/vendor/ImGuizmo/src/"
IncludeDir["yaml_cpp"] = "%{wks.location}/Saturn/vendor/yaml-cpp/include/"
IncludeDir["ImguiNodeEditor"] = "%{wks.location}/Saturn/vendor/imgui_node_editor"
IncludeDir["ImSpinner"] = "%{wks.location}/Saturn/vendor/imspinner/src"
IncludeDir["Tracy"] = "%{wks.location}/Saturn/vendor/tracy/src"
IncludeDir["Filewatch"] = "%{wks.location}/Saturn/vendor/Filewatch/src"
IncludeDir["MiniAudio"] = "%{wks.location}/Saturn/vendor/miniaudio/src"
IncludeDir["SharedStorage"] = "%{wks.location}/Saturn-SharedStorage/src"
IncludeDir["zlib"] = "%{wks.location}/Saturn/vendor/zlib"
IncludeDir["Recast"] = "%{wks.location}/Saturn/vendor/Recast/RecastAndDetour/Include"
IncludeDir["acl"] = "%{wks.location}/Saturn/vendor/acl/include"
IncludeDir["rtm"] = "%{wks.location}/Saturn/vendor/acl/rtm/include"
IncludeDir["Freetype"] = "%{wks.location}/Saturn/vendor/freetype/include"
IncludeDir["MSDF"] = "%{wks.location}/Saturn/vendor/msdf-atlas-gen/msdfgen"
IncludeDir["MSDFAG"] = "%{wks.location}/Saturn/vendor/msdf-atlas-gen"
IncludeDir["JoltPhys"] = "%{wks.location}/Saturn/vendor/JoltPhysics/Jolt"
IncludeDir["Steamworks"] = "%{wks.location}/Saturn/vendor/steamworks/Include"
IncludeDir["NativeFileDialogExtended"] = "%{wks.location}/Saturn/vendor/nativefiledialogext/src/include"
IncludeDir["ImTimeline"] = "%{wks.location}/Saturn/vendor/ImTimeline/ImTimeline/src"
IncludeDir["libzip"] = "%{wks.location}/Saturn/vendor/libzip/include"
IncludeDir["ImGuiColorTextEdit"] = "%{wks.location}/Saturn/vendor/ImGuiColorTextEdit/src"
IncludeDir["CrashCatch"] = "%{wks.location}/Saturn/vendor/CrashCatch/src"

-- // -Dependencies-- 
group "Dependencies"
	include "Saturn/vendor/imgui"
	include "Saturn/vendor/SPIRV-Cross"
	include "Saturn/vendor/yaml-cpp"
	include "Saturn/vendor/tracy"
	include "Saturn/vendor/zlib"
	include "Saturn/vendor/Recast"
	include "Saturn/vendor/freetype"
	include "Saturn/vendor/msdf-atlas-gen"
	include "Saturn/vendor/JoltPhysics"
	include "Saturn/vendor/nativefiledialogext"
	include "Saturn/vendor/ImTimeline"
-- // -Dependencies-- 

-- // -Engine-- 
group "Engine"
	include "Saturn/Saturn"
-- // -Engine-- 

-- // -Editor-- 
group "Editor"
	include "Saturn-Editor/Saturn-Editor"
-- // -Editor-- 

-- // -Project browser-- 
group "Tools"
	include "Saturn-ProjectBrowser/Saturn-ProjectBrowser"
-- // -Project browser-- 

-- // -Shared Storage-- 
group "Tools"
	include "Saturn-SharedStorage/Saturn-SharedStorage"
-- // -Shared Storage-- 

-- // -Saturn Build Tool-- 
group "Trinity"
	include "SaturnBuildTool/SBT"
-- // -Saturn Build Tool-- 

-- // -Saturn Header Tool-- 
group "Trinity"
	include "SaturnHeaderTool/SaturnHeaderTool"
-- // -Saturn Header Tool-- 
