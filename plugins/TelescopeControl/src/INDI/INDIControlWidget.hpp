#ifndef INDICONTROLWIDGET_HPP
#define INDICONTROLWIDGET_HPP

#include <QWidget>

namespace Ui {
class INDIControlWidget;
}

class INDIControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit INDIControlWidget(QWidget *parent = 0);
    ~INDIControlWidget();

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

    Ui::INDIControlWidget *ui;
};

#endif // INDICONTROLWIDGET_HPP
