import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel, QWidget, QVBoxLayout, QPushButton
from PyQt5.QtGui import QIcon, QFont
from PyQt5.QtCore import Qt


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("miencraft launcher")
        self.setWindowIcon(QIcon("assets/GrassBlock.png"))

        # Central widget (required for layouts in QMainWindow)
        central = QWidget()
        self.setCentralWidget(central)

        layout = QVBoxLayout()

        # Title label
        label = QLabel("Welcome to the miencraft launcher v0.1!")
        label.setFont(QFont("Comic Sans MS", 20))
        label.setAlignment(Qt.AlignCenter)

        label.setStyleSheet("""
            color: #91d991;
            background-color: #0f800f;
            padding: 10px;
        """)

        # Buttons
        play_button = QPushButton("Play")
        settings_button = QPushButton("Settings")
        quit_button = QPushButton("Quit")

        # Add widgets to layout
        layout.addWidget(label)
        layout.addWidget(play_button)
        layout.addWidget(settings_button)
        layout.addWidget(quit_button)

        central.setLayout(layout)


def main():
    app = QApplication(sys.argv)

    window = MainWindow()
    window.showMaximized()

    sys.exit(app.exec_())


if __name__ == '__main__':
    main()