function GetFullPath(pathStr)
	local fullPath = debug.getinfo(1).source:match("@?(.*/)")
	local endPos = string.find(fullPath, "Make")
	fullPath = string.sub(fullPath, 0, endPos - 1)
	fullPath = string.gsub(fullPath, "/", "\\")
	
	return fullPath
end

local AbsoluteRootDir = GetFullPath()
local RootDir = "../../"
local IncludeDir = RootDir ..  "ThirdParty/Include/"
local SourceFolder = "Src/"
local LibDir = RootDir .. "_Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}"
local ThirdPartyLibDir = RootDir .. "ThirdParty/Lib/x64/"

local ProjectAction = "UNDEFINED"

workspace "RZE"
	filter {}
	
	-- Argument passed in when running premake5
	if _ACTION ~= nil then
		ProjectAction = _ACTION
	end

	configurations { "Debug", "Release" }

	platforms { "x64" }
	architecture "x64";
	
	startproject "Game"
	
	-- Where the project files will be generated
	location(RootDir .. "_Project")

	filter "configurations:Debug"	
		defines { "DEBUG" } 
		symbols "Full"
	filter "configurations:Release"	
		defines { "NDEBUG" } 
		optimize "On"

	--
	-- Visual Studio
	--
	filter
	{
		"system:windows",
		"action:vs*"
	}
	
	systemversion "10.0.18362.0"
	toolset "v142"
	
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
	}

	--
	--
	-- RZE_ENGINE
	--
	--
	project "Engine"
		local ProjectDir = RootDir .. "Engine/"
		local SourceDir = ProjectDir .. SourceFolder

		filter {}
		
		flags { "FatalCompileWarnings" }
		disablewarnings { "4267" }

		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		targetdir (LibDir)
		targetname "RZE_Engine"

		dependson { "Rendering", "Utils" }

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
			RootDir .. "Modules/Rendering/Src/"
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir
		}
		links
		{
			"OptickCore",
			-- RZE --
			"RZE_Utils",
			"Rendering",
		}

		local command = "xcopy /y /d /s "
		local outputDir = "$(ProjectDir)$(OutDir)"
		postbuildcommands {
			command .. AbsoluteRootDir .. "ThirdParty\\DLL\\x64 " .. outputDir
		}
		
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
	-- RZE_SourceAssetBurner
	--
	--
	project "SourceAssetBurner"
		local ProjectDir = RootDir .. "SourceAssetBurner/"
		local SourceDir = ProjectDir .. SourceFolder

		filter {}
		
		flags { "FatalCompileWarnings" }
		disablewarnings { "4267" }

		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		targetdir (LibDir)
		targetname "RZE_SourceAssetBurner"
		
		dependson { "Utils" }

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
			RootDir .. "Utils/Src/"
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir,
		}
		
		links
		{
			"assimp",
			"RZE_Utils"
		}
		
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
		local ProjectDir = RootDir .. "Utils/"
		local SourceDir = ProjectDir .. SourceFolder

		filter {}
		
		flags { "FatalCompileWarnings" }
		disablewarnings { "4267" }

		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		targetdir (LibDir)
		targetname "RZE_Utils"

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
			ThirdPartyLibDir,
		}
		
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
	-- RENDERING
	--
	--
	 project "Rendering"
		local ProjectDir = RootDir .. "Modules/Rendering/"
		local SourceDir = ProjectDir .. SourceFolder

		filter {}
		
		flags { "FatalCompileWarnings" }
		disablewarnings { "4267" }
		
		kind "StaticLib"
		language "C++"
		cppdialect "C++17"
		targetdir (LibDir)
		targetname "Rendering"

		dependson { "Externals", "Utils" }

		files
		{
			SourceDir .. "**.h",
			SourceDir .. "**.hpp",
			SourceDir .. "**.c",
			SourceDir .. "**.cpp"
		}

		includedirs
		{
			RootDir .. "Engine/Src",
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
			"OptickCore",
			"D3d12.lib",
			"d3d11.lib",
			"DXGI.lib",
			"D3DCompiler.lib",
			"DirectXTK",
			-- RZE
			"RZE_Utils",
			"Externals"
		}

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
	 		local ProjectDir = RootDir .. "Externals/"
	 		local SourceDir = ProjectDir .. SourceFolder

			filter {}
			
	 		kind "StaticLib"
	 		language "C++"
	 		targetdir (LibDir)
	 		targetname "Externals"

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
	 			ThirdPartyLibDir,
	 		}

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
		local ProjectDir = RootDir .. "Game/"
		local SourceDir = ProjectDir

		filter {}
		
		flags { "FatalCompileWarnings" }
		disablewarnings { "4267" }
		
		kind "ConsoleApp"
		language "C++"
		cppdialect "C++17"
		targetdir (LibDir)
		targetname "RZE_Game"

		dependson { "Engine", "Rendering", "Utils"}

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
			RootDir .. "Engine/Src/",
			RootDir .. "Utils/Src/",
			RootDir .. "Modules/Rendering/Src/"
		}

		libdirs
		{
			LibDir,
			ThirdPartyLibDir,
		}
		links
		{
			"RZE_Engine",
			"RZE_Utils",
			"Rendering"
		}

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
	-- RZE_EDITOR
	--
	--
	project "Editor"
	local ProjectDir = RootDir .. "Editor/"
	local SourceDir = ProjectDir .. SourceFolder

	filter {}
	
	flags { "FatalCompileWarnings" }
	disablewarnings { "4267" }
	
	kind "ConsoleApp"
	language "C++"
	targetdir (LibDir)
	targetname "RZE_Editor"

	dependson { "Engine", "Rendering", "Utils"}

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
		RootDir .. "Engine/Src/",
		RootDir .. "Utils/Src/",
		RootDir .. "Modules/Rendering/Src/"
	}

	libdirs
	{
		LibDir,
		ThirdPartyLibDir,
	}
	links
	{
		"RZE_Engine",
		"RZE_Utils",
		"Rendering"
	}

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
