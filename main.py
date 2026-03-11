import sys
from PyQt5.QtWidgets import QApplication, QMainWindow, QLabel
from PyQt5.QtGui import QIcon

class MainWindow(QMainWindow):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("miencraft launcher")
        self.setGeometry(0,0,1800,800)
        self.setWindowIcon(QIcon("assets/GrassBlock.png"))
        self.text = "Welcome to the miencraft launcher v0.1!"
        label = QLabel(self.text, self)
        

def main():
    app = QApplication(sys.argv)
    window = MainWindow()
    window.show()
    sys.exit(app.exec_())

if __name__ == '__main__':
    main()