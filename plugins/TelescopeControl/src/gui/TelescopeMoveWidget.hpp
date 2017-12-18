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
	void moveNorth(bool active);
	void moveEast(bool active);
	void moveWest(bool active);
	void moveSouth(bool active);
	void setSpeed(int speed);

private slots:
	void onNorthButtonPressed();
	void onNorthButtonReleased();
	void onEastButtonPressed();
	void onEastButtonReleased();
	void onSouthButtonPressed();
	void onSouthButtonReleased();
	void onWestButtonPressed();
	void onWestButtonReleased();

private:
	Ui::TelescopeMoveWidget *ui;
};



#endif // TELESCOPEMOVEWIDGET_HPP
