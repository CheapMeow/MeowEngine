import os

def find_header_files(folder_path):
    header_files = []
    for root, dirs, files in os.walk(folder_path):
        for file in files:
            if file.endswith(".h") or file.endswith(".hpp"):
                header_files.append(os.path.join(root, file).replace('\\', '/'))  # Replace backslashes with forward slashes
    return header_files

def main():
    folder_path = input("Enter the folder path to search for .h or .hpp files: ")
    if os.path.isdir(folder_path):
        header_files = find_header_files(folder_path)
        if header_files:
            print("Found .h or .hpp files:")
            for file_path in header_files:
                print(file_path)
        else:
            print("No .h or .hpp files found in the specified folder.")
    else:
        print("Invalid folder path.")

if __name__ == "__main__":
    main()
