/*
 * Copyright (C) 2009 Matthew Gates
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA  02110-1335, USA.
 */
 
#ifndef _TUINODE_HPP_
#define _TUINODE_HPP_ 1

#include <QObject>
#include <QString>
#include <QList>

//! @struct TuiNodeResponse
//! A TuiNodeResponse contains a flag, "accepted" if a keystroke was accepted
//! And a link to a node, should the key action have prompted a change in the
//! current node used by the menu system.
typedef struct
{
	bool accepted;
	class TuiNode* newNode;
} TuiNodeResponse;

//! @class TuiNode
//! TuiNode objects are linked together in a network of nodes to form 
//! the structure of a menu which may be navigated using the cursor keys.
//! Each node has a single line of text which will be displayed when a
//! node is active.  Depending on the sub-class for a particular node,
//! it may be used to edit data of one sort of another.
class TuiNode : public QObject
{
	Q_OBJECT
public:
	//! Create a TuiNode
	//! @param text the text to display for this node
	//! @param parent the node for the parent menu item
	//! @param prev the previous node in the current menu (typically 
	//! shares the same parent)
	TuiNode(const QString& text, TuiNode* parent=NULL, TuiNode* prev=NULL);
	virtual TuiNodeResponse handleKey(int key);
	virtual TuiNodeResponse navigation(int key);
	virtual QString getDisplayText();
	virtual TuiNode* getParentNode() {return parentNode;}
	virtual void setParentNode(TuiNode* n) {parentNode=n; updateNodeNumber();}
	virtual TuiNode* getChildNode() {return childNode;}
	virtual void setChildNode(TuiNode* n) {childNode=n;}
	virtual TuiNode* getPrevNode() {return prevNode;}
	virtual void setPrevNode(TuiNode* n) {prevNode=n; updateNodeNumber();}
	virtual TuiNode* getNextNode() {return nextNode;}
	virtual void setNextNode(TuiNode* n) {nextNode=n;}
	//! Set prevNode to the last of the chain of nextNode-s.
	//! Call for the first node of a menu after all others have been added.
	virtual void loopToTheLast();
	
	int getNodeNumber() {return nodeNumber;}
	QList<int> getAncestorsNumbers() {return ancestorsNumbers;}

protected:	
	TuiNode* parentNode;
	TuiNode* childNode;
	TuiNode* prevNode;
	TuiNode* nextNode;
	//! Text of the prefix containing the hierarchical node number.
	QString prefixText;
	QString displayText;
	//! Number of the node in the current menu.
	//! Automatically set to 1 if there is no prevNode.
	int nodeNumber;
	//! Contains the numbers of the parent nodes in the hierarchy.
	//! The last element is the number of the node in the current menu.
	QList<int> ancestorsNumbers;
	//! Updates nodeNumber, ancestorNumbers and prefixText.
	void updateNodeNumber();
};

#endif /* _TUINODE_HPP_ */

