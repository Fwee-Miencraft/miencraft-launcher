import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QLabel, QWidget,
    QVBoxLayout, QHBoxLayout, QPushButton
)
from PyQt5.QtGui import QIcon, QFont
from PyQt5.QtCore import Qt

def setbuttoncolor(button:QPushButton, button_color_hex:str):
    #A function to help with setting background color of a Button.
    button.setStyleSheet(f"background-color: {button_color_hex};")

class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()

        self.setWindowTitle("miencraft launcher")
        self.setWindowIcon(QIcon("assets/GrassBlock.png"))
        self.setStyleSheet("background-color:#91d991")

        central = QWidget()
        self.setCentralWidget(central)

        main_layout = QVBoxLayout()
        main_layout.setSpacing(25)

        # title
        label = QLabel("Welcome to the miencraft launcher v0.1!")
        label.setFont(QFont("Comic Sans MS", 24))
        label.setAlignment(Qt.AlignCenter)

        # Buttons
        play_button = QPushButton("Play")
        settings_button = QPushButton("Settings")
        quit_button = QPushButton("Quit")

        # set background color of button using the set_button_color function
        setbuttoncolor(play_button, "#5c945c")
        setbuttoncolor(settings_button, "#5c945c")
        setbuttoncolor(quit_button, "#5c945c")

        #Make buttons bigger
        play_button.setMinimumHeight(60)
        settings_button.setMinimumHeight(50)
        quit_button.setMinimumHeight(50)

        play_button.setMinimumWidth(200)
        settings_button.setMinimumWidth(150)
        quit_button.setMinimumWidth(150)

        bottom_layout = QHBoxLayout()
        bottom_layout.setSpacing(20)

        bottom_layout.addStretch()
        bottom_layout.addWidget(settings_button)
        bottom_layout.addWidget(quit_button)
        bottom_layout.addStretch()

        # Center everything
        main_layout.addStretch()
        main_layout.addWidget(label, alignment=Qt.AlignCenter)
        main_layout.addWidget(play_button, alignment=Qt.AlignCenter)
        main_layout.addLayout(bottom_layout)
        main_layout.addStretch()

        central.setLayout(main_layout)


def main():
    app = QApplication(sys.argv)

    window = MainWindow()
    window.showMaximized()

    sys.exit(app.exec_())


if __name__ == "__main__":
    main()