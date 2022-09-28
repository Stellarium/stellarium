#ifndef TOPMOST_H
#define TOPMOST_H

#include <QWidget>

namespace Ui {
class TopMost;
}

class TopMost : public QWidget
{
	Q_OBJECT

public:
	explicit TopMost(QWidget *parent = nullptr);
	~TopMost();
};

#endif // TOPMOST_H
