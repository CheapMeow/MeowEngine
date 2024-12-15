target("code_generator", function()
    set_kind("binary")
    add_files(code_generator_dir .. "/**.cpp")
    add_includedirs(code_generator_dir)
    add_links("$(env LLVM_DIR)/lib/libclang.lib")
    add_includedirs("$(env LLVM_DIR)/include")
    add_packages("glm", "glfw", "volk", "spirv-cross", "stb", "assimp", "imgui")
    after_build(function (target)
        -- -- Get the list of header files from project_A
        -- local header_files = os.files("$(projectdir)/src/meow_runtime/**.{h,hpp}")
        -- local latest_header_time = 0
        
        -- -- Find the latest modification time of the header files
        -- for _, header in ipairs(header_files) do
        --     local mtime = os.mtime(header)
        --     if mtime > latest_header_time then
        --         latest_header_time = mtime
        --     end
        -- end
        
        -- -- Get the target file of project_A (e.g., static library file)
        -- local target_file_A = target:targetfile("project_A")
        
        -- -- Check if the target file for project_A exists
        -- local target_file_exists = os.exists(target_file_A)
        -- local target_file_time = target_file_exists and os.mtime(target_file_A) or 0
        
        -- -- Get the modification time of project_B's output file
        -- local target_file_B = target:targetfile()
        -- local target_file_B_time = os.mtime(target_file_B)
        
        -- -- Rebuild project_B if project_A's target file is missing or if any header files were modified
        -- -- or if the latest header modification time is newer than project_B's target file modification time
        -- if not target_file_exists or latest_header_time > target_file_B_time then
        --     print("Rebuilding project_B due to updated headers or missing target in project_A.")
        --     target:rebuild()  -- Trigger the rebuild of project_B
        -- end
        
        local vulkan_sdk = os.getenv("VULKAN_SDK")
        local llvm_dir = os.getenv("LLVM_DIR")
        local include_dir_list = {}
        table.insert(include_dir_list, "-I" .. vulkan_sdk .. "/Include")
        table.insert(include_dir_list, "-I" .. target:pkg("glfw"):installdir() .. "/include")
        table.insert(include_dir_list, "-I" .. target:pkg("glm"):installdir())
        table.insert(include_dir_list, "-I" .. target:pkg("volk"):installdir())
        table.insert(include_dir_list, "-I" .. target:pkg("spirv-cross"):installdir())
        table.insert(include_dir_list, "-I" .. target:pkg("stb"):installdir())
        table.insert(include_dir_list, "-I" .. target:pkg("assimp"):installdir() .. "/include")
        table.insert(include_dir_list, "-I" .. target:pkg("imgui"):installdir())
        table.insert(include_dir_list, "-I" .. llvm_dir .. "/include")
        table.insert(include_dir_list, "-I" .. llvm_dir .. "/lib/clang/19/include")
        table.insert(include_dir_list, "-I" .. os.projectdir() .. "/src/meow_runtime")
        
        local src_path = path.join("-S" .. os.projectdir(), "src/meow_runtime");
        local output_path = path.join("-O" .. os.projectdir(), "src/meow_runtime/generated");
        
        local args = include_dir_list
        table.insert(args, src_path)
        table.insert(args, output_path)

        os.execv(target:targetfile(), args)
    end)
end)
