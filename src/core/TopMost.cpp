#include "TopMost.hpp"

TopMost::TopMost(QWidget *parent) :
	QWidget(parent)
{
	setAttribute(Qt::WA_TranslucentBackground, true);
	setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowTransparentForInput);
	setGeometry(0,0,1,1);
}

TopMost::~TopMost()
{

}
