#include <QApplication>
#include <QWidget>
#include <QDialog>

#include <stdio.h>
#include <windows.h>

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QDialog dialog;
  dialog.setWindowTitle("Example Dialog");
  dialog.exec();
}
