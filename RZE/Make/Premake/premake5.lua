function GetFullPath(pathStr)
	local fullPath = debug.getinfo(1).source:match("@?(.*/)")
	local endPos = string.find(fullPath, "Make")
	fullPath = string.sub(fullPath, 0, endPos - 1)
	fullPath = string.gsub(fullPath, "/", "\\")
	
	return fullPath .. "Engine\\"
end

local AbsoluteRootDir = GetFullPath()
local RootDir = "../../"
local ProjectAction = "UNDEFINED"

workspace "RZE"
	-- Argument passed in when running premake5
	if _ACTION ~= nil then
		ProjectAction = _ACTION
	end

	configurations { "Debug", "Release" }

	platforms { "x64" }
	architecture "x64";
	
	startproject "Editor"
	
	-- Where the project files will be generated
	location(RootDir .. "_Project")

	filter "configurations:Debug"	defines { "DEBUG" } symbols "On"
	filter "configurations:Release"	defines { "NDEBUG" } optimize "On"

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
		"/ignore:4099",
		"/ignore:4221",
		"/ignore:4006"
	};

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
		local BinaryDir = ProjectDir .. "ThirdPary/DLL/"
		local LibDir = RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		local ThirdPartyLibDir = ProjectDir .. "ThirdParty/Lib/x64/"

		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Engine"

		dependson { "Apollo", "Diotima", "Perseus", "Externals", "Utils" }

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
			SourceDir,
			RootDir .. "Utils/Src/",
			RootDir .. "Modules/Apollo/Src/",
			RootDir .. "Modules/Diotima/Src/",
			RootDir .. "Modules/Perseus/Src/"
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir
		}
		links
		{
			"OpenGL32",
			"assimp64",
			"glew64",
			"ProfilerCore64",
			-- RZE --
			"RZE_Utils",
			"Apollo",
			"Diotima",
			"Perseus",
			"Externals"
		}

		local command = "xcopy /y /d "
		local outputDir = "$(ProjectDir)$(OutDir)"
		postbuildcommands {
			command .. AbsoluteRootDir .. "ThirdParty\\DLL\\x64 " .. outputDir
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
		local SourceDir = ProjectDir .. "Src/"
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = EngineDir .. "ThirdParty/Lib/x64/"

		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Utils"

		pchheader = "Utils/StdAfx.h"
		pchsource = (SourceDir .. "Utils/StdAfx.cpp")

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
			RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
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
	-- APOLLO
	--
	--
	project "Apollo"
		local EngineDir = RootDir .. "Engine/"
		local ProjectDir = RootDir .. "Modules/Apollo/"
		local SourceDir = ProjectDir .. "Src/"
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		local ThirdPartyLibDir = EngineDir .. "ThirdParty/Lib/x64/"

		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "Apollo"

		dependson { "Perseus", "Utils" }

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
			IncludeDir,
			SourceDir,
			EngineDir .. "/Src",
			RootDir .. "Utils/Src/",
			RootDir .. "Modules/Perseus/Src/"
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir
		}
		links
		{
			-- ThirdParty
			"OpenGL32",
			"assimp64",
			"glew64",
			"ProfilerCore64",
			-- RZE
			"Perseus",
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
	-- PERSEUS
	--
	--
	project "Perseus"
		local EngineDir = RootDir .. "Engine/"
		local ProjectDir = RootDir .. "Modules/Perseus/"
		local SourceDir = ProjectDir .. "Src/"
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		local ThirdPartyLibDir = EngineDir .. "ThirdParty/Lib/x64/"

		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "Perseus"

		dependson { "Utils" }

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
			RootDir .. "Utils/Src/",
			SourceDir,
			IncludeDir
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir
		}
		links
		{
			-- ThirdParty
			"OpenGL32",
			"assimp64",
			"glew64",
			"ProfilerCore64",
			-- RZE
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
		local LibDir = RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		local ThirdPartyLibDir = EngineDir .. "ThirdParty/Lib/x64/"

		kind "StaticLib"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "Diotima"

		dependson { "Perseus", "Utils" }

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
			RootDir .. "Utils/Src/",
			RootDir .. "Modules/Perseus/Src/",
			SourceDir,
			IncludeDir
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir
		}
		links
		{
			-- ThirdParty
			"OpenGL32",
			"assimp64",
			"glew64",
			"ProfilerCore64",
			-- RZE
			"Perseus",
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
	 	-- Externals
	 	--
	 	--
	 	project "Externals"
	 		local EngineDir = RootDir .. "Engine/"
	 		local ProjectDir = RootDir .. "Externals/"
	 		local SourceDir = ProjectDir .. "Src/"
	 		local IncludeDir = EngineDir .. "ThirdParty/Include/"
	 		local LibDir = EngineDir .. "ThirdParty/Lib/x64/"

	 		kind "StaticLib"
	 		language "C++"
	 		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
	 		targetname "Externals"

	 		pchheader = "Utils/StdAfx.h"
	 		pchsource = (SourceDir .. "Utils/StdAfx.cpp")

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
	 			RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
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
		local LibDir = EngineDir .. "ThirdParty/Lib/x64/"

		kind "ConsoleApp"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Game"

		dependson { "Engine", "Apollo", "Diotima", "Perseus", "Utils"}

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
			ProjectDir,
			EngineDir .. "/Src/",
			RootDir .. "/Utils/Src/",
			RootDir .. "/Modules/Apollo/Src/",
			RootDir .. "/Modules/Diotima/Src/",
			RootDir .. "/Modules/Perseus/Src/",
		}

		libdirs
		{
			LibDir,
			RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		}
		links
		{
			"RZE_Engine",
			"RZE_Utils",
			"Apollo",
			"Diotima",
			"Perseus",
			"Externals"
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
	-- RZE_EDITORPROJ
	--
	--
	 project "Editor"
		local EngineDir = RootDir .. "Engine/"
		local ProjectDir = RootDir .. "Editor/"
		local SourceDir = ProjectDir
		local IncludeDir = EngineDir .. "ThirdParty/Include/"
		local LibDir = EngineDir .. "ThirdParty/Lib/x64/"

		kind "ConsoleApp"
		language "C++"
		targetdir (RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}")
		targetname "RZE_Editor"

		dependson { "Engine", "Apollo", "Diotima", "Perseus", "Utils"}

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
			ProjectDir,
			EngineDir .. "/Src/",
			RootDir .. "/Utils/Src/",
			RootDir .. "/Modules/Apollo/Src/",
			RootDir .. "/Modules/Diotima/Src/",
			RootDir .. "/Modules/Perseus/Src/"
		}

		libdirs
		{
			LibDir,
			RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
		}
		links
		{
			"RZE_Engine",
			"RZE_Utils",
			"Apollo",
			"Diotima",
			"Perseus",
			"Externals"
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