project "VulkanCore"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    targetdir "Binaries/%{cfg.buildcfg}"
    staticruntime "off"

    files {
        "Include/**.h", "Include/**.hpp",
        "Source/**.c", "Source/**.cpp",
    }

   includedirs {
        "Include",
        "%{wks.location}/Vendor/glfw-3.4/include",
        "%{wks.location}/Vendor/glm",
        "%{wks.location}/Vendor/stb",
        "%{wks.location}/Vendor/assimp/include",
        "%{wks.location}/Vendor/assimp/build/include",
        "%{wks.location}/Vendor/meshoptimizer/src",
        "%{wks.location}/Vendor/assimp/contrib/zlib",
        os.getenv("VULKAN_SDK") .. "/Include"
   }

   libdirs {
      "%{wks.location}/Vendor/glfw-3.4/lib",
      "%{wks.location}/Vendor/assimp/build/lib",
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
       defines { "GLM_ENABLE_EXPERIMENTAL" }

   filter "configurations:Debug"
       defines { "DEBUG" }
       runtime "Debug"
       symbols "On"
       libdirs { "%{wks.location}/Vendor/assimp/build/lib/Debug" }
       links { "assimp-vc143-mtd" }

   filter "configurations:Release"
       defines { "RELEASE" }
       runtime "Release"
       optimize "On"
       symbols "On"
       libdirs { "%{wks.location}/Vendor/assimp/build/lib/Release" }
       links { "assimp-vc143-mt" }

   filter "configurations:Dist"
       defines { "DIST" }
       runtime "Release"
       optimize "On"
       symbols "Off"
       libdirs { "%{wks.location}/Vendor/assimp/build/lib/Release" } 
       links { "assimp-vc143-mt" }