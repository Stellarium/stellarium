#include "SeasonsWidget.hpp"
#include "StelApp.hpp"
#include "StelCore.hpp"
#include "StelModuleMgr.hpp"
#include "StelUtils.hpp"
#include "SpecificTimeMgr.hpp"
#include "StelLocaleMgr.hpp"

#include <QPushButton>
#include <QToolTip>
#include <QSettings>

SeasonsWidget::SeasonsWidget(QWidget* parent)
	: QWidget(parent)
	, ui(new Ui_seasonsWidget)
{
}

void SeasonsWidget::setup()
{
	ui->setupUi(this);
	connect(&StelApp::getInstance(), &StelApp::languageChanged, this, &SeasonsWidget::retranslate);

	core = StelApp::getInstance().getCore();
	specMgr = GETSTELMODULE(SpecificTimeMgr);
	localeMgr = &StelApp::getInstance().getLocaleMgr();

	connect(core, SIGNAL(locationChanged(StelLocation)), this, SLOT(setSeasonLabels()));
	connect(core, SIGNAL(dateChangedByYear(const int)), this, SLOT(setSeasonTimes()));
	connect(specMgr, SIGNAL(eventYearChanged()), this, SLOT(setSeasonTimes()));

	connect(ui->buttonMarchEquinoxCurrent, &QPushButton::clicked, this, [=](){specMgr->currentMarchEquinox();});
	connect(ui->buttonMarchEquinoxNext, &QPushButton::clicked, this, [=](){specMgr->nextMarchEquinox();});
	connect(ui->buttonMarchEquinoxPrevious, &QPushButton::clicked, this, [=](){specMgr->previousMarchEquinox();});

	connect(ui->buttonSeptemberEquinoxCurrent, &QPushButton::clicked, this, [=](){specMgr->currentSeptemberEquinox();});
	connect(ui->buttonSeptemberEquinoxNext, &QPushButton::clicked, this, [=](){specMgr->nextSeptemberEquinox();});
	connect(ui->buttonSeptemberEquinoxPrevious, &QPushButton::clicked, this, [=](){specMgr->previousSeptemberEquinox();});

	connect(ui->buttonJuneSolsticeCurrent, &QPushButton::clicked, this, [=](){specMgr->currentJuneSolstice();});
	connect(ui->buttonJuneSolsticeNext, &QPushButton::clicked, this, [=](){specMgr->nextJuneSolstice();});
	connect(ui->buttonJuneSolsticePrevious, &QPushButton::clicked, this, [=](){specMgr->previousJuneSolstice();});

	connect(ui->buttonDecemberSolsticeCurrent, &QPushButton::clicked, this, [=](){specMgr->currentDecemberSolstice();});
	connect(ui->buttonDecemberSolsticeNext, &QPushButton::clicked, this, [=](){specMgr->nextDecemberSolstice();});
	connect(ui->buttonDecemberSolsticePrevious, &QPushButton::clicked, this, [=](){specMgr->previousDecemberSolstice();});

	populate();

	QSize button = QSize(24, 24);
	ui->buttonMarchEquinoxCurrent->setFixedSize(button);
	ui->buttonMarchEquinoxNext->setFixedSize(button);
	ui->buttonMarchEquinoxPrevious->setFixedSize(button);
	ui->buttonJuneSolsticeCurrent->setFixedSize(button);
	ui->buttonJuneSolsticeNext->setFixedSize(button);
	ui->buttonJuneSolsticePrevious->setFixedSize(button);
	ui->buttonSeptemberEquinoxCurrent->setFixedSize(button);
	ui->buttonSeptemberEquinoxNext->setFixedSize(button);
	ui->buttonSeptemberEquinoxPrevious->setFixedSize(button);
	ui->buttonDecemberSolsticeCurrent->setFixedSize(button);
	ui->buttonDecemberSolsticeNext->setFixedSize(button);
	ui->buttonDecemberSolsticePrevious->setFixedSize(button);
}

void SeasonsWidget::retranslate()
{
	ui->retranslateUi(this);
	setSeasonTimes();
}

void SeasonsWidget::populate()
{
	// Set season labels
	setSeasonLabels();
	// Compute equinoxes/solstices and fill the time
	setSeasonTimes();
}

void SeasonsWidget::setSeasonLabels()
{
	const float latitide = StelApp::getInstance().getCore()->getCurrentLocation().getLatitude();
	if (latitide >= 0.f)
	{
		// Northern Hemisphere
		// TRANSLATORS: The name of season
		ui->labelMarchEquinox->setText(qc_("Spring","season"));
		// TRANSLATORS: The name of season
		ui->labelJuneSolstice->setText(qc_("Summer","season"));
		// TRANSLATORS: The name of season
		ui->labelSeptemberEquinox->setText(qc_("Fall","season"));
		// TRANSLATORS: The name of season
		ui->labelDecemberSolstice->setText(qc_("Winter","season"));
	}
	else
	{
		// Southern Hemisphere
		// TRANSLATORS: The name of season
		ui->labelMarchEquinox->setText(qc_("Fall","season"));
		// TRANSLATORS: The name of season
		ui->labelJuneSolstice->setText(qc_("Winter","season"));
		// TRANSLATORS: The name of season
		ui->labelSeptemberEquinox->setText(qc_("Spring","season"));
		// TRANSLATORS: The name of season
		ui->labelDecemberSolstice->setText(qc_("Summer","season"));
	}
}

void SeasonsWidget::setSeasonTimes()
{
	const double JD = core->getJD() + core->getUTCOffset(core->getJD()) / 24;
	int year, month, day;
	double jdFirstDay, jdLastDay;
	StelUtils::getDateFromJulianDay(JD, &year, &month, &day);
	StelUtils::getJDFromDate(&jdFirstDay, year, 1, 1, 0, 0, 0);
	StelUtils::getJDFromDate(&jdLastDay, year, 12, 31, 24, 0, 0);
	const double marchEquinox = specMgr->getEquinox(year, SpecificTimeMgr::Equinox::March);
	const double septemberEquinox = specMgr->getEquinox(year, SpecificTimeMgr::Equinox::September);
	const double juneSolstice = specMgr->getSolstice(year, SpecificTimeMgr::Solstice::June);
	const double decemberSolstice = specMgr->getSolstice(year, SpecificTimeMgr::Solstice::December);
	QString days = qc_("days", "duration");
	int jdDepth = 5;
	int daysDepth = 2;

	// Current year
	ui->labelCurrentYear->setText(QString::number(year));
	ui->labelYearDuration->setText(QString("(%1 %2)").arg(QString::number(jdLastDay-jdFirstDay), days));
	// Spring/Fall
	ui->labelMarchEquinoxJD->setText(QString::number(marchEquinox, 'f', jdDepth));
	ui->labelMarchEquinoxLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(marchEquinox), localeMgr->getPrintableTimeLocal(marchEquinox)));
	ui->labelMarchEquinoxDuration->setText(QString("%1 %2").arg(QString::number(juneSolstice-marchEquinox, 'f', daysDepth), days));
	// Summer/Winter
	ui->labelJuneSolsticeJD->setText(QString::number(juneSolstice, 'f', jdDepth));
	ui->labelJuneSolsticeLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(juneSolstice), localeMgr->getPrintableTimeLocal(juneSolstice)));
	ui->labelJuneSolsticeDuration->setText(QString("%1 %2").arg(QString::number(septemberEquinox-juneSolstice, 'f', daysDepth), days));
	// Fall/Spring
	ui->labelSeptemberEquinoxJD->setText(QString::number(septemberEquinox, 'f', jdDepth));
	ui->labelSeptemberEquinoxLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(septemberEquinox), localeMgr->getPrintableTimeLocal(septemberEquinox)));
	ui->labelSeptemberEquinoxDuration->setText(QString("%1 %2").arg(QString::number(decemberSolstice-septemberEquinox, 'f', daysDepth), days));
	// Winter/Summer
	ui->labelDecemberSolsticeJD->setText(QString::number(decemberSolstice, 'f', jdDepth));
	ui->labelDecemberSolsticeLT->setText(QString("%1 %2").arg(localeMgr->getPrintableDateLocal(decemberSolstice), localeMgr->getPrintableTimeLocal(decemberSolstice)));
	const double duration = (marchEquinox-jdFirstDay) + (jdLastDay-decemberSolstice);
	ui->labelDecemberSolsticeDuration->setText(QString("%1 %2").arg(QString::number(duration, 'f', daysDepth), days));
}
