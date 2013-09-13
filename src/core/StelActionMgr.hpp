
#include <QObject>
#include <QKeySequence>
#include <QList>

class StelAction : public QObject
{
	Q_OBJECT
public:
	friend class StelActionMgr;
	Q_PROPERTY(bool checked READ isChecked WRITE setChecked NOTIFY toggled)

	//! Don't use this constructor, this is just there to ease the migration from QAction.
	StelAction(QObject *parent): QObject(parent) {}

	StelAction(const QString& actionId,
			   const QString& groupId,
			   const QString& text,
			   const QString& primaryKey,
			   bool global = false);
	//! Connect the action to an object property or slot.
	//! @param slot A property or a slot name.  The slot can either have the signature `func()`, and in that
	//! case the action is made not checkable, either have the signature `func(bool)` and in that case the action
	//! is made checkable.  When linked to a property the action is always made checkable.
	void connectToObject(QObject* obj, const char* slot);
	//! Don't use setCheckable, connectToObject can automatically determine if the action is checkable or not.
	//! This is just there to ease the migration from QAction.
	void setCheckable(bool value) {checkable = value;}
	bool isCheckable() const {return checkable;}
	bool isChecked() const {return checked;}
	void setShortcut(const QString& key);
	void setAltShortcut(const QString& key);
	QKeySequence::SequenceMatch matches(const QKeySequence& seq) const;

	QString getId() const {return objectName();}
	QString getGroup() const {return group;}
	const QKeySequence getShortcut() const {return keySequence;}
	const QKeySequence getAltShortcut() const {return altKeySequence;}
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
	QString group;
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

	QStringList getGroupList() const;
	QList<StelAction*> getActionList(const QString& group) const;

	//! Save current shortcuts to file.
	void saveShortcuts();
	//! Restore the default shortcuts combinations
	void restoreDefaultShortcuts();

public slots:
	//! Enable/disable all actions of application.
	//! need for editing shortcuts without trigging any actions
	//! @todo find out if this is really necessary and why.
	void setAllActionsEnabled(bool value) {actionsEnabled = value;}
private:
	bool actionsEnabled;
	QList<int> keySequence;
};
