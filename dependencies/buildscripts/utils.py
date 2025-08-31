import os
import sys
import subprocess
from pathlib import Path

def fail_exit(message):
    print(f"Error: {message}")
    sys.exit(1)

def ensure_directory(path_str):
    path = Path(path_str)
    if not path.exists():
        try:
            path.mkdir(parents = True)
            print(f"Created directory: {path}")
        except Exception as e:
            fail_exit(f"Failed to create directory '{path}': {e}")
    return path.resolve()

def change_directory(path):
    try:
        os.chdir(path)
    except Exception as e:
        fail_exit(f"Failed to change directory: {e}")

def replace_in_file(filepath, old_text, new_text):
    try:
        with open(filepath, "r", encoding = "utf-8") as file:
            content = file.read()

        updated_content = content.replace(old_text, new_text)

        with open(filepath, "w", encoding = "utf-8") as file:
            file.write(updated_content)
    except FileNotFoundError:
        print(f"File not found: {filepath}")
    except Exception as e:
        print(f"Error while replacing text in file: {e}")

def run_command(command_list):
    try:
        print(command_list)
        subprocess.run(command_list, check = True)
    except subprocess.CalledProcessError as e:
        fail_exit(f"Command failed with exit code {e.returncode}")
    except FileNotFoundError:
        fail_exit(f"Command not found: {command_list[0]}")
