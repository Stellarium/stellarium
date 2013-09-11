
#include <QObject>
#include <QKeySequence>
#include <QList>

class StelAction : public QObject
{
	Q_OBJECT
public:
	friend class StelActionMgr;
	Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)

	StelAction(const QString& actionId,
			   const QString& groupId,
			   const QString& text,
			   const QString& primaryKey,
			   bool global = false);
	void connectToObject(QObject* obj, const char* slot);
	bool isCheckable() const {return checkable;}
	bool isChecked() const {return checked;}
	void setShortcut(const QString& key);
	void setAltShortcut(const QString& key);
	QKeySequence::SequenceMatch matches(const QKeySequence& seq) const;
	const QKeySequence getShortcut() const {return keySequence;}
	const QString& getText() const {return text;}
signals:
	void toggled(bool);
	void triggered();
public slots:
	void setChecked(bool);
	void trigger();
	void toggle();
private slots:
	void propertyChanged(bool);
private:
	bool checkable;
	bool checked;
	QString text;
	bool global;
	QKeySequence keySequence;
	QKeySequence altKeySequence;
	QObject* target;
	const char* property;
};

class StelActionMgr : public QObject
{
	Q_OBJECT
public:
	StelActionMgr();
	~StelActionMgr();
	//! Create and add a new StelAction, connected to an object property or slot.
	//! @param id Global identifier.
	//!	@param groupId Group identifier.
	//! @param text Short human-readable description in English.
	//! @param shortcut Default shortcut.
	//! @param target The QObject the action is linked to.
	//! @param slot Either a slot name, in that case the action is not checkable,
	//! either a property name, in that case the action is checkable.
	StelAction* addAction(const QString& id, const QString& groupId, const QString& text,
						  const QString& shortcut, QObject* target, const char* slot,
						  bool global=false);
	StelAction* findAction(const QString& id);
	bool pushKey(int key, bool global=false);
private:
	QList<int> keySequence;
};
