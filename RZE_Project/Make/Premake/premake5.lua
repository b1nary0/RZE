local RootDir = "../../"
local ProjectAction = "UNDEFINED"

workspace "RZE"
	configurations { "Debug", "Release" }
	
	platforms { "x32" }
	
	-- Argument passed in when running premake5
	if _ACTION ~= nill then 
		ProjectAction = _ACTION;
	end
	
	-- Where the project files will be generated
	location(RootDir .. "Project_" .. ProjectAction);
	
	flags "FatalWarnings"
	warnings "Extra";
	
	filter "configurations:Debug"	defines { "DEBUG" } symbols "On";
	filter "configurations:Release"	defines { "NDEBUG" } optimize "On";
	
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
	
	filter {}
	
	--
	--
	-- RZE_ENGINE
	--
	--
	project "Engine"
		local ProjectDir = RootDir .. "RZE_Engine/";
		local SourceDir = ProjectDir .. "Src/";
		
		kind "ConsoleApp";
		language "C++";
		targetdir (ProjectDir .. "Build/" .. "%{cfg.buildcfg}/" .. "%{cfg.platform}");
		targetname "RZE_Engine";
		
		files
		{
			SourceDir .. "**.h",
			SourceDir .. "**.hpp",
			SourceDir .. "**.c",
			SourceDir .. "**.cpp"
		};
		
		filter {};
		
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




















