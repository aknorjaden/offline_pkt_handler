/*
	------------------------------------------------------------------------------------
	LICENSE:
	------------------------------------------------------------------------------------
	This file is part of EVEmu: EVE Online Server Emulator
	Copyright 2006 - 2011 The EVEmu Team
	For the latest information visit http://evemu.org
	------------------------------------------------------------------------------------
	This program is free software; you can redistribute it and/or modify it under
	the terms of the GNU Lesser General Public License as published by the Free Software
	Foundation; either version 2 of the License, or (at your option) any later
	version.

	This program is distributed in the hope that it will be useful, but WITHOUT
	ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
	FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more details.

	You should have received a copy of the GNU Lesser General Public License along with
	this program; if not, write to the Free Software Foundation, Inc., 59 Temple
	Place - Suite 330, Boston, MA 02111-1307, USA, or go to
	http://www.gnu.org/copyleft/lesser.txt.
	------------------------------------------------------------------------------------
	Author:		Captnoord
*/

/* this is done using pure evil bleh, it needs some more cleaning */
class util_CachedObject : public PyClass
{
protected:
public:
    util_CachedObject() : PyClass("util.CachedObject") {}

    ~util_CachedObject(){};
    void destruct() {}

    util_CachedObject* New()
    {
        return new util_CachedObject();
    }
    
    /* comments: format guessed from compiled scripts */
    bool init(PyObject* state)
    {
        if (!PyTuple_Check(state))
            return false;

        PyTuple * pState = (PyTuple*)state;

        if (pState->size() != 3)
            return false;

        mDict->set_item("objectID", pState->GetItem(0));
        mDict->set_item("nodeID", pState->GetItem(1));
        mDict->set_item("objectVersion", pState->GetItem(2));
        return true;
    }

    /* this is a stub that needs to be implemented */
    PyTuple* GetState()
    {
        return NULL;
    }
};
