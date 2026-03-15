import customtkinter as ctk
from PIL import Image, ImageTk
import sys
import os
import requests
import platform
import zipfile
import io
import subprocess
import shutil

# --- INITIAL SETUP ---
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

app = ctk.CTk()
app.title("Miencraft Launcher")
app.geometry("1000x600")
app.attributes("-topmost", True)
app.after(500, lambda: app.attributes("-topmost", False))
app.after(100, lambda: (app.lift(), app.focus_force()))

# ---- SYSTEM DETECTION ----
system_type = platform.system()

# --- CENTER WINDOW ---
app.update_idletasks()
screen_width = app.winfo_screenwidth()
screen_height = app.winfo_screenheight()
x = (screen_width // 2) - 500
y = (screen_height // 2) - 300
app.geometry(f"1000x600+{x}+{y}")

# --- ICON ---
def set_icon():
    icon_path = os.path.join("assets", "GrassBlock.png")
    if os.path.exists(icon_path):
        try:
            img = Image.open(icon_path).convert("RGBA")
            app.icon_object = ImageTk.PhotoImage(img)
            app.wm_iconphoto(False, app.icon_object)
        except Exception as e:
            print(f"Icon error: {e}")
    else:
        print(f"Icon not found: {icon_path}")

app.after(200, set_icon)

# --- TITLE ---
title = ctk.CTkLabel(
    app,
    text="Miencraft Launcher",
    font=("Georgia", 48, "bold")
)
title.pack(pady=60)

# --- HELPERS ---

def download_and_extract_repo():
    url = "https://github.com/Fwee-Miencraft/miencraft/archive/refs/heads/main.zip"
    extract_dir = "miencraft-game"  # clean name

    print("Downloading repository ZIP...")
    r = requests.get(url, stream=True)
    r.raise_for_status()

    # Clear old folder if exists
    if os.path.exists(extract_dir):
        shutil.rmtree(extract_dir)

    with zipfile.ZipFile(io.BytesIO(r.content)) as z:
        # Extract directly, strip top-level folder if needed
        for member in z.namelist():
            filename = member
            # Remove the top-level "miencraft-main/" prefix
            if filename.startswith("miencraft-main/"):
                filename = filename[len("miencraft-main/"):]
            target_path = os.path.join(extract_dir, filename)
            os.makedirs(os.path.dirname(target_path), exist_ok=True)
            if not member.endswith('/'):
                z.extract(member, extract_dir)
                # Rename extracted file to correct path
                old_path = os.path.join(extract_dir, member)
                if os.path.exists(old_path):
                    shutil.move(old_path, target_path)

    print(f"Game extracted to: {os.path.abspath(extract_dir)}")

def launch_game():
    game_folder = "miencraft-game"
    
    # Get absolute path to the game folder
    base_dir = os.path.abspath(os.path.dirname(__file__))  # folder where launcher is
    full_game_folder = os.path.join(base_dir, game_folder)
    
    if not os.path.exists(full_game_folder):
        print(f"Game folder not found: {full_game_folder}")
        return

    if system_type == "Darwin":
        executable = os.path.join(full_game_folder, "main")
        if not os.path.exists(executable):
            print(f"Executable not found: {executable}")
            print("Make sure you extracted the ZIP and the 'main' file is inside miencraft-game/")
            return
        
        try:
            os.chmod(executable, 0o755)  # make sure it's executable
            print(f"Launching: {executable}")
            print(f"Working directory: {full_game_folder}")
            
            # Use absolute path + correct cwd
            subprocess.Popen([executable], cwd=full_game_folder)
        except Exception as e:
            print(f"Launch failed: {e}")
    else:
        executable = os.path.join(full_game_folder, "main.exe")
        if not os.path.exists(executable):
            print(f"Executable not found: {executable}")
            return
        print(f"Launching: {executable}")
        subprocess.Popen(executable, cwd=full_game_folder)

# --- BUTTON FRAME ---
button_frame = ctk.CTkFrame(app, fg_color="transparent")
button_frame.pack(pady=20)

def launch():
    print("Launch button pressed")
    download_and_extract_repo()
    launch_game()

launch_button = ctk.CTkButton(
    button_frame,
    text="Launch Miencraft",
    font=("Courier New", 22, "bold"),
    width=350,
    height=70,
    command=launch
)
launch_button.pack(pady=20)

quit_button = ctk.CTkButton(
    button_frame,
    text="Quit",
    font=("Courier New", 22, "bold"),
    width=350,
    height=70,
    fg_color="#2f2f2f",
    hover_color="#3a3a3a",
    command=sys.exit
)
quit_button.pack(pady=20)

# --- VERSION LABEL ---
version = ctk.CTkLabel(
    app,
    text="Miencraft Launcher • v0.1",
    font=("Segoe UI", 14)
)
version.place(relx=0.98, rely=0.97, anchor="se")

app.mainloop()