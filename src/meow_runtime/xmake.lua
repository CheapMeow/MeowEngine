target("meow_runtime", function()
    set_kind("static")
    add_files(runtime_dir .. "/**.cpp")
    add_includedirs(runtime_dir, {public = true})
    add_packages("glm", "glfw", "vulkansdk", "volk", "spirv-cross", "stb", "assimp", "imgui", {public = true})

    add_defines("ENGINE_ROOT_DIR=\"" .. os.projectdir() .. "\"", {public = true})
    
    add_defines("NOMINMAX", {public = true}) -- for std::numeric_limits<double>::min() and max()

    add_defines("GLM_ENABLE_EXPERIMENTAL", {public = true})   -- for GLM: GLM_GTX_component_wise
    add_defines("GLFW_INCLUDE_VULKAN", {public = true})
    add_defines("GLFW_EXPOSE_NATIVE_WIN32", {public = true}) -- TODO: Config by platform
    add_defines("VK_USE_PLATFORM_WIN32_KHR", {public = true}) -- TODO: Config by platform
    if is_mode("EditorDebug", "GameDebug") then
        add_defines("VKB_DEBUG", "VKB_VALIDATION_LAYERS", {public = true})
    end
    add_defines("VK_NO_PROTOTYPES", {public = true})
    add_defines("IMGUI_DEFINE_MATH_OPERATORS", {public = true})

    -- if is_plat("windows") and is_tool("msvc") then
    --     add_cxflags("/permissive-", {public = true})
    --     add_cxflags("/WX-", {public = true})
    -- end
end)