project "VulkanCore"
   kind "StaticLib"
   language "C++"
   cppdialect "C++20"
   targetdir "Binaries/%{cfg.buildcfg}"
   staticruntime "off"

   files { "Source/**.h", "Source/**.cpp" }

   includedirs {
      "Include",
      "%{wks.location}/Vendor/glfw-3.4/include",
      "%{wks.location}/Vendor/glm",
      "%{wks.location}/Vendor/stb",
      "%{wks.location}/Vendor/assimp/include",
      "%{wks.location}/Vendor/meshoptimizer",
      os.getenv("VULKAN_SDK") .. "/Include"
   }

   libdirs {
      "%{wks.location}/Vendor/glfw-3.4/lib",
      os.getenv("VULKAN_SDK") .. "/Lib"
   }

   links {
       "glfw3",
       "vulkan-1",
       "SPIRVd",
       "SPIRV-Toolsd",
       "SPIRV-Tools-diffd",
       "SPIRV-Tools-optd",
       "glslangd",
       "OSDependentd",
       "GenericCodeGend",
       "MachineIndependentd",
       "glslang-default-resource-limitsd"
   }

   targetdir ("../Binaries/" .. OutputDir .. "/%{prj.name}")
   objdir ("../Binaries/Intermediates/" .. OutputDir .. "/%{prj.name}")

   filter "system:windows"
       systemversion "latest"
       defines { }

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