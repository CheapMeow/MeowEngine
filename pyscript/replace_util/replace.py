import os

def remove_prefix(s, prefix):  
    if s.startswith(prefix):  
        return s[len(prefix):]  
    return s  

def find_header_files(folder_path, prefix_to_add=""):
    prefix = folder_path + "/"
    header_files = []
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".h") or file.endswith(".hpp"):
                header_file = os.path.join(root, file).replace('\\', '/')
                header_file = prefix_to_add + remove_prefix(header_file, prefix)
                header_files.append(header_file)  # Replace backslashes with forward slashes
    return header_files

def find_source_files(folder_path, prefix_to_add=""):
    prefix = folder_path + "/"
    source_files = []
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".cpp"):
                source_file = os.path.join(root, file).replace('\\', '/')
                source_file = prefix_to_add + remove_prefix(source_file, prefix)
                source_files.append(source_file)  # Replace backslashes with forward slashes
    return source_files

def replace_headers_and_sources(cmakelist_root, header_root, src_root, header_prefix="", source_prefix=""):
    if not os.path.isdir(cmakelist_root):
        return
    
    template_file_path = cmakelist_root + "/CMakeLists.txt.template"
    if not os.path.exists(cmakelist_root + "/CMakeLists.txt.template"):
        return
    
    with open(template_file_path, "r", encoding="utf-8") as file:  
        file_content = file.read()  

    placeholder = "<all_headers_place_holder>"
    if placeholder in file_content:
        header_files = find_header_files(header_root, header_prefix)
        all_headers = "\n".join([f"    {header}" for header in header_files])
        
        file_content = file_content.replace(placeholder, all_headers)  
    
    placeholder = "<all_sources_place_holder>"
    if placeholder in file_content:
        source_files = find_source_files(src_root, source_prefix)
        all_sources = "\n".join([f"    {source}" for source in source_files])
        
        file_content = file_content.replace(placeholder, all_sources)  
    
    with open(cmakelist_root + "/CMakeLists.txt", "w", encoding="utf-8") as file:  
        file.write(file_content)