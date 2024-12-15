set_project("MeowEngine")
set_languages("c++20")

-- 设置全局的第三方库路径
src_dir = os.projectdir() .. "/src"
code_generator_dir = os.projectdir() .."/src/code_generator"
runtime_dir = os.projectdir() .. "/src/meow_runtime"
editor_dir = os.projectdir() .. "/src/meow_editor"

rule("EditorDebug", function()
    after_load(function(target)
        if is_mode("debug") then
            if not target:get("symbols") then
                target:set("symbols", "debug")
            end
            if not target:get("optimize") then
                target:set("optimize", "none")
            end
            target:set("targetdir", "build/EditorDebug")
            target:set("defines", { "MEOW_EDITOR", "MEOW_DEBUG" })
        end
    end)
end)

rule("EditorRelease", function()
    after_load(function(target)
        if is_mode("EditorRelease") then
            if not target:get("symbols") and target:targetkind() ~= "shared" then
                target:set("symbols", "hidden")
            end
            if not target:get("optimize") then
                if is_plat("android", "iphoneos") then
                    target:set("optimize", "smallest")
                else
                    target:set("optimize", "fastest")
                end
            end
            if not target:get("strip") then
                target:set("strip", "all")
            end
            target:set("targetdir", "build/EditorRelease")
            target:set("defines", { "MEOW_EDITOR" })
        end
    end)
end)


add_rules("EditorDebug", "EditorRelease")

add_requires("vulkansdk")
-- add_requires("llvm") llvm isn't supported well now, so hard code dependency on env
add_requires("glm", "glfw", "volk", "spirv-cross", "stb", "assimp")
add_requires("imgui docking", {configs = {glfw = true, vulkan = true}})

includes(code_generator_dir)
includes(runtime_dir)
includes(editor_dir)