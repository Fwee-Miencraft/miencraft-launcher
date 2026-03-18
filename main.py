import customtkinter as ctk #type:ignore
from PIL import Image, ImageTk
from updater import Updater
import sys, os, platform, subprocess, threading
from tkinter import messagebox

# Make the updater Object
updater = Updater()

# --- INITIAL SETUP ---
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

app = ctk.CTk()
app.title("Miencraft Launcher")
app.geometry("1000x600")

# ---- SYSTEM DETECTION ----
system_type = platform.system()

# --- CENTER WINDOW ---
app.update_idletasks()
screen_width = app.winfo_screenwidth()
screen_height = app.winfo_screenheight()
x = (screen_width // 2) - 500
y = (screen_height // 2) - 300
app.geometry(f"1000x600+{x}+{y}")

# --- UI ELEMENTS FOR UPDATER ---
# These are hidden by default and shown during download
progress_label = ctk.CTkLabel(app, text="Checking for updates...", font=("Segoe UI", 14))
progress_label.pack(pady=(10, 0))

progress_bar = ctk.CTkProgressBar(app, width=400)
progress_bar.set(0)
progress_bar.pack(pady=10)

# --- UPDATER LOGIC ---
def update_progress_ui(value):
    """Callback function to update the progress bar from the thread"""
    progress_bar.set(value)
    percentage = int(value * 100)
    progress_label.configure(text=f"Downloading Update: {percentage}%")

def run_update_check():
    """Background task to handle updates without freezing the UI"""
    online_version = updater.get_online_version()
    
    if online_version is None:
        # 1. Show warning popup when no wifi/server down
        messagebox.showwarning("Connection Error", "Could not reach update server. Starting in offline mode.")
        progress_label.pack_forget()
        progress_bar.pack_forget()
        return

    local_version = updater.get_local_version()
    
    if local_version != online_version:
        # 2. Show progress bar and download
        try:
            updater.download_update(progress_callback=update_progress_ui)
            progress_label.configure(text="Update Successful!")
            # Hide progress bar after 3 seconds
            app.after(3000, lambda: [progress_bar.pack_forget(), progress_label.pack_forget()])
        except Exception as e:
            messagebox.showerror("Update Failed", f"An error occurred: {e}")
    else:
        # Already up to date, hide the bars
        progress_label.pack_forget()
        progress_bar.pack_forget()

# Start update check in a separate thread
update_thread = threading.Thread(target=run_update_check, daemon=True)
app.after(100, update_thread.start)

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

# --- GAME LAUNCHER ---
def launch_game():
    game_folder = "miencraft-game"
    base_dir = os.path.abspath(os.path.dirname(__file__))
    full_game_folder = os.path.join(base_dir, game_folder)

    if not os.path.exists(full_game_folder):
        messagebox.showerror("Error", "Game files not found. Please connect to internet to download. Will try to download again.")
        # Try to reinstall The Exacutable if it was deleted by someone like me...
        try:
            updater.download_update(progress_callback=update_progress_ui)
        except Exception as e:
            print(f"Error: {e}")
        launch_game()
        return

    if system_type == "Darwin":
        # Look for Miencraft.app bundle inside the extracted folder
        app_bundle_name = "Miencraft.app"  # case-sensitive — adjust if different
        executable_path = os.path.join(full_game_folder, app_bundle_name)

        if not os.path.exists(executable_path):
            other_exacutable_path = os.path.join(full_game_folder, "miencraft-mac", app_bundle_name)
            if os.path.exists(other_exacutable_path):
                executable_path = other_exacutable_path
            else:
                messagebox.showerror("Launch Error", 
                    "Miencraft.app bundle not found in miencraft-game.\n"
                    "Expected: miencraft-game/Miencraft.app or miencraft-game/miencraft-mac/Miencraft.app"
                )
                return

        executable = os.path.join(
            executable_path,
            "Contents",
            "MacOS",
            "Miencraft"
        )
        executable_path = os.path.join(
            executable_path,
            "Contents",
            "Macos"
        )

        if os.path.exists(executable):
            os.chmod(executable, 0o755)
            subprocess.Popen([executable], cwd=executable_path)
        else:
            print(f"Exacutable not found: {executable}")

    elif system_type == "Windows":
        path_to_game = os.path.join(full_game_folder, "miencraft-win")
        executable = os.path.join(path_to_game, "main.exe")
        if os.path.exists(executable):
            subprocess.Popen([executable], cwd=path_to_game)

# --- BUTTON FRAME ---
button_frame = ctk.CTkFrame(app, fg_color="transparent")
button_frame.pack(pady=20)

launch_button = ctk.CTkButton(
    button_frame,
    text="Launch Miencraft",
    font=("Courier New", 22, "bold"),
    width=350,
    height=70,
    command=launch_game
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
version_lbl = ctk.CTkLabel(
    app,
    text="Miencraft Launcher • v0.1",
    font=("Segoe UI", 14)
)
version_lbl.place(relx=0.98, rely=0.97, anchor="se")

try:
    app.mainloop()
except KeyboardInterrupt:
    sys.exit()