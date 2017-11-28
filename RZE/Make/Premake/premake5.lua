local RootDir = "../../"
local ProjectAction = "UNDEFINED"

workspace "RZE"
	configurations { "Debug", "Release" }
	
	platforms { "x32" }
	
	-- Argument passed in when running premake5
	if _ACTION ~= nil then 
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
		"MultiProcessorCompile",
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
		local ProjectDir = RootDir .. "Engine/"
		local SourceDir = ProjectDir .. "Src/"
		local IncludeDir = ProjectDir .. "ThirdParty/Include/"
		local LibDir = RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		local ThirdPartyLibDir = ProjectDir .. "ThirdParty/Lib/x86/"
		
		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Engine"
		
		dependson { "Utils" }
		
		pchheader "StdAfx.h"
		pchsource "../../Engine/Src/StdAfx.cpp"
		
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
			"../../Utils/Src/",
			SourceDir
		}
		
		libdirs
		{
			LibDir,
			ThirdPartyLibDir
		}
		links
		{
			"assimp",
			"freetyped",
			"glew32s",
			"imgui",
			"OpenGL32",
			"RZE_Utils"
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
	-- DIOTIMA
	--
	--
	 project "Diotima"
		local EngineDir = RootDir .. "Engine/"
		local ProjectDir = RootDir .. "Modules/Diotima/"
		local SourceDir = ProjectDir .. "Src/"
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = EngineDir .. "ThirdParty/Lib/x86/"
		
		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "Diotima"
		
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
			SourceDir,
			IncludeDir
		}
		
		libdirs
		{
			LibDir,
			RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		}
		links
		{
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
	
	--
	--
	-- RZE_Utils
	--
	--
	project "Utils"
		local EngineDir = RootDir .. "Engine/"
		local ProjectDir = RootDir .. "Utils/"
		local SourceDir = ProjectDir .. "Src/Utils/"
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = EngineDir .. "ThirdParty/Lib/x86/"
		
		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Utils"
		
		pchheader = "StdAfx.h"
		pchsource = (SourceDir .. "StdAfx.cpp")
		
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
			SourceDir
		}
		
		libdirs
		{
			LibDir,
			RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		}
		links
		{
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
		local EngineDir = RootDir .. "Engine/"
		local ProjectDir = RootDir .. "Game/"
		local SourceDir = ProjectDir
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = EngineDir .. "ThirdParty/Lib/x86/"
		
		kind "ConsoleApp"
		language "C++"
		targetdir (RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Game"
		
		dependson { "Engine", "Utils"}
		
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
			EngineDir .. "/Src/",
			RootDir .. "/Utils/Src/",
			IncludeDir,
			IncludeDir .. "FreeType/"
		}
		
		libdirs
		{
			LibDir,
			RootDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		}
		links
		{
			"RZE_Engine",
			"RZE_Utils"
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
	 



















