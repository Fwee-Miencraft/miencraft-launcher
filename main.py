import customtkinter as ctk
from PIL import Image, ImageTk  # Added ImageTk here
import sys
import os

# --- INITIAL SETUP ---
ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

app = ctk.CTk()
app.title("Minecraft Launcher")
app.geometry("1000x600")

# --- CROSS-PLATFORM ICON LOGIC ---
def set_icon():
    icon_path = os.path.join("assets", "GrassBlock.png")
    if os.path.exists(icon_path):
        try:
            # 1. Open with PIL
            img = Image.open(icon_path).convert("RGBA")
            
            # 2. Convert PIL image to a format Tkinter/CTk understands
            # We keep a reference (app.icon_object) so it doesn't get deleted by Python
            app.icon_object = ImageTk.PhotoImage(img)
            
            # 3. Set the icon
            app.wm_iconphoto(False, app.icon_object)
            
        except Exception as e:
            print(f"Icon error: {e}")
    else:
        print(f"File not found: {icon_path}")

# Slight delay to ensure the window is initialized
app.after(200, set_icon)

# --- CENTER WINDOW ---
app.update_idletasks()
screen_width = app.winfo_screenwidth()
screen_height = app.winfo_screenheight()
x = (screen_width // 2) - 500
y = (screen_height // 2) - 300
app.geometry(f"1000x600+{x}+{y}")

# ---- TITLE ----
title = ctk.CTkLabel(
    app,
    text="Miencraft Launcher",
    font=("Georgia", 48, "bold")
)
title.pack(pady=60)

# ---- BUTTON FRAME ----
button_frame = ctk.CTkFrame(app, fg_color="transparent")
button_frame.pack(pady=20)

def launch():
    print("Launch button pressed")

launch_button = ctk.CTkButton(
    button_frame,
    text="Launch Game",
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

# ---- VERSION LABEL ----
version = ctk.CTkLabel(
    app,
    text="Minecraft Launcher, v0.1",
    font=("Segoe UI", 14)
)
version.place(relx=0.98, rely=0.97, anchor="se")

app.mainloop()