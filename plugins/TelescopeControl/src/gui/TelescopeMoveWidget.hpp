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
	void moveNorth(int speed);
	void moveEast(int speed);
	void moveWest(int speed);
	void moveSouth(int speed);
	void move(double angle, double speed);

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
	double speed() const;

	Ui::TelescopeMoveWidget *ui;
};



#endif // TELESCOPEMOVEWIDGET_HPP
