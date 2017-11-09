local RootDir = "../../"
local ProjectAction = "UNDEFINED"

workspace "RZE"
	configurations { "Debug", "Release" }
	
	platforms { "x32" }
	
	-- Argument passed in when running premake5
	if _ACTION ~= nill then 
		ProjectAction = _ACTION
	end
	
	-- Where the project files will be generated
	location(RootDir .. "_Project")
	
	--flags "FatalWarnings"
	
	filter "configurations:Debug"	defines { "DEBUG" } symbols "On"
	filter "configurations:Release"	defines { "NDEBUG" } optimize "On"
	
	filter
	{
		"platforms:*32"
	}
	architecture "x86";
	
	--
	-- Visual Studio
	--
	filter
	{
		"system:windows",
		"action:vs*"
	}
	flags
	{
		--"MultiProcessorCompile",
		"NoMinimalRebuild"
	}
	linkoptions
	{
		"/ignore:4099"
	};
	
	startproject "Game"
	
	filter {}
	
	--
	--
	-- RZE_ENGINE
	--
	--
	project "Engine"
		local ProjectDir = RootDir .. "RZE_Engine/"
		local SourceDir = ProjectDir .. "Src/"
		local IncludeDir = ProjectDir .. "ThirdParty/Include/"
		local LibDir = ProjectDir .. "ThirdParty/Lib/x86/"
		
		kind "StaticLib"
		language "C++"
		targetdir (ProjectDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Engine"
		
		pchheader "StdAfx.h"
		pchsource "RZE_Engine/Src/StdAfx.cpp"
		
		buildoptions
		{
			'/Yc"Stdafx.h" /Fp"obj\\x32\\Debug\\RZE_Engine.pch"'
		}
		
		files
		{
			SourceDir .. "**.h",
			SourceDir .. "**.hpp",
			SourceDir .. "**.c",
			SourceDir .. "**.cpp"
		}
		
		includedirs
		{
			IncludeDir,
			IncludeDir .. "FreeType/",
			SourceDir
		}
		
		libdirs
		{
			LibDir
		}
		links
		{
			"assimp",
			"freetyped",
			"glew32s",
			"imgui",
			"OpenGL32"
		}
		
		filter {}
		
		 vpaths 
		 {
			["Source Files/*"] = 
			{ 
				SourceDir .. "**.h",
				SourceDir .. "**.hpp",
				SourceDir .. "**.c",
				SourceDir .. "**.cpp"
			}
		 } 
	
	--
	--
	-- RZE_GAMEPROJ
	--
	--
	 project "Game"
		local EngineDir = RootDir .. "RZE_Engine/"
		local ProjectDir = RootDir .. "RZE_GameProj/"
		local SourceDir = ProjectDir
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = EngineDir .. "ThirdParty/Lib/x86/"
		
		kind "ConsoleApp"
		language "C++"
		targetdir (ProjectDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Game"
		
		dependson { "Engine" }
		
		filter "action:vs*"
			pchheader = "StdAfx.h"
			pchsource = "StdAfx.cpp"
		
		files
		{
			SourceDir .. "**.h",
			SourceDir .. "**.hpp",
			SourceDir .. "**.c",
			SourceDir .. "**.cpp"
		}
		
		includedirs
		{
			EngineDir .. "/Src",
			IncludeDir,
			IncludeDir .. "FreeType/"
		}
		
		libdirs
		{
			LibDir,
			EngineDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		}
		links
		{
			"assimp",
			"freetyped",
			"glew32s",
			"imgui",
			"OpenGL32",
			"RZE_Engine.lib"
		}
		
		filter {}
		
		 vpaths 
		 {
			["Source Files/*"] = 
			{ 
				SourceDir .. "**.h",
				SourceDir .. "**.hpp",
				SourceDir .. "**.c",
				SourceDir .. "**.cpp"
			}
		 }
	 



















