#include "painterdemo.h"
#include <QtGui/QApplication>

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	PainterDemo w;
	w.show();
	return a.exec();
}
