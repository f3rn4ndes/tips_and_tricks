import os

def rename_all_folders(base_path, old_char, new_char):
    try:
        # Traverse through all directories and subdirectories
        for root, dirs, _ in os.walk(base_path):
            for folder_name in dirs:
                folder_path = os.path.join(root, folder_name)
                
                # Check if the folder name contains the old_char
                if old_char in folder_name:
                    new_folder_name = folder_name.replace(old_char, new_char)
                    new_folder_path = os.path.join(root, new_folder_name)
                    
                    # Rename folder
                    os.rename(folder_path, new_folder_path)
                    print(f"Renamed: '{folder_path}' -> '{new_folder_path}'")
    except Exception as e:
        print(f"An error occurred: {e}")

# Define base path and characters for renaming
base_path = r"D:\eng4sys\projetos\021.24.MEG\EM - Especificação de Materiais e Equipamentos\MUDAR"  # Change this to your desired base path
old_char = "."  # Character to replace
new_char = "-"  # Replacement character

# Rename all folders
rename_all_folders(base_path, old_char, new_char)
