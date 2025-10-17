project "VulkanApp"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp", "Shaders/**.frag", "Shaders/**.vert"}

   includedirs {
      "Source",
	  "../Core/Include",
      "%{wks.location}/Vendor/glfw-3.4/include",
      "%{wks.location}/Vendor/assimp/include",
      "%{wks.location}/Vendor/glm",
      os.getenv("VULKAN_SDK") .. "/Include"
   }

   libdirs {
      "%{wks.location}/Vendor/glfw-3.4/lib",
      "%{wks.location}/Vendor/assimp/build/lib",
      os.getenv("VULKAN_SDK") .. "/Lib"
   }

   links {
      "VulkanCore",
      "glfw3",
      "vulkan-1",
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { "GLM_ENABLE_EXPERIMENTAL" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
       libdirs { "%{wks.location}/Binaries/Debug", "%{wks.location}/Vendor/assimp/build/lib/Debug" }
       links { "assimp-vc143-mtd" }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       libdirs { "%{wks.location}/Binaries/Release", "%{wks.location}/Vendor/assimp/build/lib/Release" }
       links { "assimp-vc143-mt" }

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"
       libdirs { "%{wks.location}/Binaries/Dist", "%{wks.location}/Vendor/assimp/build/lib/Release" }
       links { "assimp-vc143-mt" }