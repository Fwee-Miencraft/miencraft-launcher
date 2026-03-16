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
        try:
            with open(self.local_version_file, "r") as f:
                data = json.load(f)
                return data["version"]
        except:
            return None

    def get_online_version(self):
        try:
            r = requests.get(self.online_version_url, timeout=5)
            r.raise_for_status()
            return r.json()["version"]
        except Exception:
            return None

    def download_update(self, progress_callback=None):
        # We use stream=True so we can read the file in chunks
        r = requests.get(self.download_url, stream=True)
        r.raise_for_status()

        total_size = int(r.headers.get('content-length', 0))
        downloaded = 0
        buffer = io.BytesIO()

        # Download in chunks that are more bigmac
        for chunk in r.iter_content(chunk_size=1048576):
            if chunk:
                buffer.write(chunk)
                downloaded += len(chunk)
                # If a callback was provided, send the progress percentage (0.0 to 1.0)
                if progress_callback and total_size > 0:
                    progress_callback(downloaded / total_size)

        if os.path.exists(self.game_folder):
            shutil.rmtree(self.game_folder)

        # Extract from the memory buffer
        with zipfile.ZipFile(buffer) as z:
            z.extractall(self.game_folder)

        # Update local version file
        online_version = self.get_online_version()
        if online_version:
            with open(self.local_version_file, "w") as f:
                json.dump({"version": online_version}, f)

    def check_for_updates(self):
        # This is now handled by the thread in main.py, 
        # but kept for compatibility
        local = self.get_local_version()
        online = self.get_online_version()
        if online and local != online:
            self.download_update()