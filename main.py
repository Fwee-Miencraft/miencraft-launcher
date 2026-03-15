import customtkinter as ctk #type:ignore
from PIL import Image, ImageTk
from updater import Updater
import sys, os, platform, subprocess



# Make da updater Object
updater = Updater()


# --- INITIAL SETUP ---
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

app = ctk.CTk()
app.title("Miencraft Launcher")
app.geometry("1000x600")

# Run update check shortly after launcher starts
app.after(100, updater.check_for_updates)

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

# --- GAME LAUNCHER ---

def launch_game():
    game_folder = "miencraft-game"

    base_dir = os.path.abspath(os.path.dirname(__file__))
    full_game_folder = os.path.join(base_dir, game_folder)

    if not os.path.exists(full_game_folder):
        print(f"Game folder not found: {full_game_folder}")
        return

    if system_type == "Darwin":
        full_game_folder = os.path.join(full_game_folder, "miencraft-mac")
        executable = os.path.join(full_game_folder, "main")

        if not os.path.exists(executable):
            print(f"Executable not found: {executable}")
            return

        try:
            os.chmod(executable, 0o755)
            print(f"Launching: {executable}")
            subprocess.Popen([executable], cwd=full_game_folder)
        except Exception as e:
            print(f"Launch failed: {e}")

    if system_type == "Windows":
        full_game_folder = os.path.join(full_game_folder, "miencraft-win")
        executable = os.path.join(full_game_folder, "main.exe")

        if not os.path.exists(executable):
            print(f"Executable not found: {executable}")
            return

        print(f"Launching: {executable}")
        subprocess.Popen([executable])


# --- BUTTON FRAME ---
button_frame = ctk.CTkFrame(app, fg_color="transparent")
button_frame.pack(pady=20)

def launch():
    print("Launch button pressed")
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

try:
    app.mainloop()
except KeyboardInterrupt:
    sys.exit()