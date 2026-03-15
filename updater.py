import shutil, json, os, requests, zipfile, io
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
