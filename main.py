import customtkinter as ctk
import subprocess
import sys

ctk.set_appearance_mode("dark")
ctk.set_default_color_theme("dark-blue")

app = ctk.CTk()
app.title("Miencraft Launcher")
app.geometry("1000x600")
app.resizable(True, True)

# Center window
screen_width = app.winfo_screenwidth()
screen_height = app.winfo_screenheight()
x = (screen_width // 2) - 500
y = (screen_height // 2) - 300
app.geometry(f"1000x600+{x}+{y}")

# ---- TITLE ----
title = ctk.CTkLabel(
    app,
    text="Miencraft",
    font=("Georgia", 48, "bold")
)
title.pack(pady=60)

# ---- BUTTON FRAME ----
button_frame = ctk.CTkFrame(app, fg_color="transparent")
button_frame.pack(pady=20)

def quit():
    sys.exit()

def launch():
    #subprocess.Popen(["game/miencraft.exe"])
    print("no game")

launch_button = ctk.CTkButton(
    button_frame,
    text="Launch Game",
    font=("Segoe UI", 22, "bold"),
    width=350,
    height=70,
    command=launch
)
launch_button.pack(pady=20)

quit_button = ctk.CTkButton(
    button_frame,
    text="Quit",
    font=("Segoe UI", 22, "bold"),
    width=350,
    height=70,
    fg_color="#2f2f2f",
    hover_color="#3a3a3a",
    command=quit
)
quit_button.pack(pady=20)

# ---- VERSION LABEL ----
version = ctk.CTkLabel(
    app,
    text="Miencraft Launcher, v0.1",
    font=("Segoe UI", 14)
)
version.place(relx=0.98, rely=0.97, anchor="se")

app.mainloop()