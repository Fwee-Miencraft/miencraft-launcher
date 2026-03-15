import customtkinter as ctk
from PIL import Image, ImageTk
import sys, os, requests, platform, zipfile, io, subprocess, shutil, json


class Updater:
    def __init__(self):
        self.local_version_file = "version.json"
        self.online_version_url = "https://raw.githubusercontent.com/Fwee-Miencraft/miencraft/main/version.json"
        self.download_url = "https://github.com/Fwee-Miencraft/miencraft/releases/download/v0.0.1-alpha/miencraft-win.zip"
        self.game_folder = "miencraft-game"

    def get_local_version(self):
        if not os.path.exists(self.local_version_file):
            return None

        with open(self.local_version_file, "r") as f:
            data = json.load(f)
            return data["version"]

    def get_online_version(self):
        try:
            r = requests.get(self.online_version_url, timeout=5)
            r.raise_for_status()
            data = r.json()
            return data["version"]

        except requests.exceptions.RequestException:
            print("No internet connection or update server unreachable.")
            return None

    def download_update(self):
        print("Downloading update...")
        r = requests.get(self.download_url)
        r.raise_for_status()

        if os.path.exists(self.game_folder):
            shutil.rmtree(self.game_folder)

        print("Installing update...")
        with zipfile.ZipFile(io.BytesIO(r.content)) as z:
            z.extractall(self.game_folder)

        online_version = self.get_online_version()

        if online_version is not None:
            with open(self.local_version_file, "w") as f:
                json.dump({"version": online_version}, f)

            print(f"Local version updated to {online_version}")

        print("Update complete")

    def check_for_updates(self):
        local = self.get_local_version()
        online = self.get_online_version()

        if online is None:
            print("Skipping update check (offline).")
            return

        if local != online:
            print("Update available!")
            self.download_update()
        else:
            print("Game is up to date.")

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