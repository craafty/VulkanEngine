project "VulkanApp"
   kind "ConsoleApp"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   includedirs {
      "Source",
	  "../Core/Include",
      "%{wks.location}/Vendor/glfw-3.4/include",
      "%{wks.location}/Vendor/glm",
      os.getenv("VULKAN_SDK") .. "/Include"
   }

   libdirs {
      "%{wks.location}/Vendor/glfw-3.4/lib",
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
       defines { "WINDOWS" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"