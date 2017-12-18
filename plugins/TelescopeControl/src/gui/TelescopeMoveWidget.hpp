#ifndef TELESCOPEMOVEWIDGET_HPP
#define TELESCOPEMOVEWIDGET_HPP

#include <QWidget>

namespace Ui {
class TelescopeMoveWidget;
}

class TelescopeMoveWidget : public QWidget
{
	Q_OBJECT

public:
	explicit TelescopeMoveWidget(QWidget *parent = 0);
	~TelescopeMoveWidget();

signals:
	void moveNorth();
	void moveEast();
	void moveWest();
	void moveSouth();
	void setSpeed(int speed);

private:
	Ui::TelescopeMoveWidget *ui;
};

#endif // TELESCOPEMOVEWIDGET_HPP
