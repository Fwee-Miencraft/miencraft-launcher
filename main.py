import customtkinter as ctk
from PIL import Image, ImageTk
import sys, os, requests, platform, zipfile, io, subprocess, shutil, json

LOCAL_VERSION_FILE = "version.json"
ONLINE_VERSION_URL = "https://raw.githubusercontent.com/Fwee-Miencraft/miencraft/main/version.json"
# Make sure not the change the version.json until release is ready

def check_connection(url='http://www.google.com/', timeout=5):
    try:
        _ = requests.get(url, timeout=timeout)
        return True
    except requests.ConnectionError:
        return False

def get_local_version():
    if not os.path.exists(LOCAL_VERSION_FILE):
        return None

    with open(LOCAL_VERSION_FILE, "r") as f:
        data = json.load(f)
        return data["version"]

def get_online_version():
    try:
        r = requests.get(ONLINE_VERSION_URL, timeout=5)
        r.raise_for_status()
        data = r.json()
        return data["version"]

    except requests.exceptions.RequestException:
        print("No internet connection or update server unreachable.")
        return None

def download_update():
    url = "https://github.com/Fwee-Miencraft/miencraft/releases/download/v0.0.1-alpha/miencraft-win.zip"

    print("Downloading update...")
    r = requests.get(url)
    r.raise_for_status()

    # remove old game
    if os.path.exists("miencraft-game"):
        shutil.rmtree("miencraft-game")

    print("Installing update...")
    with zipfile.ZipFile(io.BytesIO(r.content)) as z:
        z.extractall("miencraft-game")

    print("Update complete")

    # Update local version.json
    online_version = get_online_version()  # get the version from online
    with open(LOCAL_VERSION_FILE, "w") as f:
        json.dump({"version": online_version}, f)
    print(f"Local version updated to {online_version}")
    print("Update complete")
#check for updates:
def check_for_updates():
    local = get_local_version()
    online = get_online_version()

    if online is None:
        print("Skipping update check (offline).")
        return

    if local != online:
        print("Update available!")
        download_update()
    else:
        print("Game is up to date.")


def install_update(zip_data):
    if os.path.exists("miencraft-game"):
        shutil.rmtree("miencraft-game")
    with zipfile.ZipFile(io.BytesIO(zip_data)) as z:
        z.extractall("miencraft-game")

# --- INITIAL SETUP ---
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

app = ctk.CTk()
app.title("Miencraft Launcher")
app.geometry("1000x600")
app.after(100, check_for_updates)

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


def launch_game():
    game_folder = "miencraft-game"
    
    # Get absolute path to the game folder
    base_dir = os.path.abspath(os.path.dirname(__file__))  # folder where launcher is
    full_game_folder = os.path.join(base_dir, game_folder)
    
    if not os.path.exists(full_game_folder):
        print(f"Game folder not found: {full_game_folder}")
        return

    if system_type == "Darwin":
        full_game_folder = os.path.join(full_game_folder, "miencraft-mac")
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
