import os

def find_cpp_files(folder_path):
    cpp_files = []
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".cpp"):
                cpp_files.append(os.path.join(root, file).replace('\\', '/'))  # Replace backslashes with forward slashes
    return cpp_files

def main():
    folder_path = input("Enter the folder path to search for .cpp files: ")
    if os.path.isdir(folder_path):
        cpp_files = find_cpp_files(folder_path)
        if cpp_files:
            print("Found .cpp files:")
            for file_path in cpp_files:
                print(file_path)
        else:
            print("No .cpp files found in the specified folder.")
    else:
        print("Invalid folder path.")

if __name__ == "__main__":
    main()
