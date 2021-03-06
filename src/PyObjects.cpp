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

#include "EvemuPCH.h"

#include "Common.h"
#include "NGLog.h"
#include "Log.h"
#include "string_map.h"

#include "PyObjects.h"
#include "PyChameleon.h"
#include "PyObjectDumper.h"

/************************************************************************/
/* PyInt                                                                */
/************************************************************************/
PyInt::PyInt( int32 num ) : PyObject(PyTypeInt), mNumber(num) {}

PyInt & PyInt::operator=( const int num )
{
	mNumber = num;
	return *this;
}

// its a weak hash I know...... take a look at this... can't remember the python stuff
uint32 PyInt::hash()
{
	return mNumber;
}

int32 PyInt::GetValue()
{
	return mNumber;
}

/************************************************************************/
/* PyLong                                                               */
/************************************************************************/
PyLong::PyLong( int64 & num ) : PyObject(PyTypeLong), mNumber(num) {}

/* @todo solve the signed/unsigned problem */
PyLong::PyLong( uint64 & num ) : PyObject(PyTypeLong), mNumber(num) {}

int64 PyLong::GetValue()
{
	return mNumber;
}

typedef uint16 digit;
#define PyLong_SHIFT    15
#define PyLong_BASE     ((digit)1 << PyLong_SHIFT)
#define PyLong_MASK     ((int)(PyLong_BASE - 1))

uint32 PyLong::hash()
{
	uint32 x;
	int64 i;
	int sign;
	uint8 * henk = (uint8*)&mNumber;

	/* This is designed so that Python ints and longs with the
	same value hash to the same value, otherwise comparisons
	of mapping keys will turn out weird */
	i = mNumber;
	sign = 1;
	x = 0;
	if (i < 0) {
		sign = -1;
		i = -(i);
	}
#define LONG_BIT_PyLong_SHIFT	(8*sizeof(uint32) - PyLong_SHIFT)
	/* The following loop produces a C long x such that (unsigned long)x
	is congruent to the absolute value of v modulo ULONG_MAX.  The
	resulting x is nonzero if and only if v is. */
	while (--i >= 0) {
		/* Force a native long #-bits (32 or 64) circular shift */
		x = ((x << PyLong_SHIFT) & ~PyLong_MASK) | ((x >> LONG_BIT_PyLong_SHIFT) & PyLong_MASK);
		x += henk[i];//v->ob_digit[i];
		/* If the addition above overflowed (thinking of x as
		unsigned), we compensate by incrementing.  This preserves
		the value modulo ULONG_MAX. */
		if ((unsigned long)x < henk[i])//v->ob_digit[i])
			x++;
	}
#undef LONG_BIT_PyLong_SHIFT
	x = x * sign;
	if (x == -1)
		x = -2;
	return x;
}

/************************************************************************/
/* PyFloat                                                              */
/************************************************************************/
PyFloat::PyFloat() : PyObject(PyTypeReal), mNumber(0.0) {}
PyFloat::PyFloat( float num ) : PyObject(PyTypeReal), mNumber(num) {}
PyFloat::PyFloat( double num ) : PyObject(PyTypeReal), mNumber(num) {}

double PyFloat::GetValue()
{
	return mNumber;
}

// hashing a float? lol....
uint32 PyFloat::hash()
{
	ASCENT_HARDWARE_BREAKPOINT;
}

/************************************************************************/
/* PyBool                                                               */
/************************************************************************/
PyBool::PyBool( bool check ) : PyObject(PyTypeBool),  mCheck(check) {}

bool PyBool::operator==( const bool check )
{
	return (mCheck == check);
}

PyBool::~PyBool()
{

}

uint32 PyBool::hash()
{
    ASCENT_HARDWARE_BREAKPOINT;
}



/************************************************************************/
/* PyTuple                                                              */
/************************************************************************/
PyTuple::PyTuple() : PyObject(PyTypeTuple) {}

PyTuple::PyTuple( size_t elementCount ) : PyObject(PyTypeTuple)
{
	if (elementCount > PY_TUPLE_ELEMENT_MAX)
	{
		Log.Error("PyTuple", "constructor is requested to allocate a stupid amount of elements: %d", elementCount);
		assert(false);
		return;
	}
	
	resize(elementCount);
}

PyTuple::~PyTuple()
{
	clear();
}

/* this is kinda slow because we are resizing a vector */
PyChameleon & PyTuple::operator[]( const int index )
{
	if (index < 0)
		return PyErrorIterator;
	if (index+1 > (int)mTuple.size())
		mTuple.resize(index+1);
	PyChameleon & itr = *mTuple[index];
	return itr;
}

size_t PyTuple::size()
{
	return mTuple.size();
}

void PyTuple::clear()
{
	iterator itr = mTuple.begin();
	for (; itr != mTuple.end(); itr++)
	{
		PyChameleon* obj = *itr;
		SafeDelete(obj);
	}
	mTuple.clear();
}

uint32 PyTuple::hash()
{
    PyTuple & tuple = *(PyTuple*)this;
    uint32 hash = 5381;
    for (int i = 0; i < (int)tuple.size(); i++)
    {
        uint32 hashChunk = PyObject_Hash(tuple[i].getPyObject());
        hash = (hash << 3) + hashChunk;
    }
    return hash;
}

/* quite tricky */
bool PyTuple::resize( size_t newTupleSize )
{
	size_t currentTupleSize = mTuple.size();

	/* if we are required to resize to our current size, don't do shit*/
	if (newTupleSize == currentTupleSize)
		return true;
	
	/* we need to grow or shrink */
	if (newTupleSize > currentTupleSize)
	{
		// debug breakpoint for the situation we are resizing a already excisting object
		if (currentTupleSize != 0)
			ASCENT_HARDWARE_BREAKPOINT;
		
		/* we need to grow (this one is easy) */
		mTuple.resize(newTupleSize);

		for (size_t i = currentTupleSize; i < newTupleSize; i++)
		{
			mTuple[int(i)] = new PyChameleon();
		}
	}
	else
	{
		ASCENT_HARDWARE_BREAKPOINT;
		/* we need to shrink (a bit harder) */
		for(size_t i = currentTupleSize; i > newTupleSize; i--)
		{
			PyChameleon* obj = mTuple[int(i)];
			SafeDelete(obj);
		}
		mTuple.resize(newTupleSize);
	}
	return true;
}

PyObject* PyTuple::GetItem( const int index )
{
	if (index < 0)
		return NULL;
	if (index+1 > (int)mTuple.size())
		mTuple.resize(index+1);

	PyChameleon * itr = mTuple[index];
	return itr->getPyObject();
}

int32 PyTuple::GetItem_asInt( const int index )
{
	if (index < 0)
		return 0;
	if (index > (int)mTuple.size())
		return 0;

	PyChameleon * itr = mTuple[index];
	if (itr == NULL)
		return 0;
	PyObject* obj = itr->getPyObject();
	if (obj == NULL || obj->gettype() != PyTypeInt)
		return 0;

	return ((PyInt*)obj)->GetValue();
}

PyTuple* PyTuple::GetItem_asPyTuple( const int index )
{
	if (index < 0)
		return NULL;
	if (index > (int)mTuple.size())
		return NULL;

	PyChameleon * itr = mTuple[index];
	if (itr == NULL)
		return NULL;
	PyObject* obj = itr->getPyObject();
	if (obj == NULL || PyTuple_Check(obj))
		return NULL;

	return (PyTuple*)obj;
}

PyList* PyTuple::GetItem_asPyList( const int index )
{
	if (index < 0)
		return NULL;
	if (index > (int)mTuple.size())
		return NULL;

	PyChameleon * itr = mTuple[index];
	if (itr == NULL)
		return NULL;
	PyObject* obj = itr->getPyObject();
	if (obj == NULL || obj->gettype() != PyTypeList)
		return NULL;

	return (PyList*)obj;
}

PyString* PyTuple::GetItem_asPyString( const int index )
{
	if (index < 0)
		return NULL;
	if (index > (int)mTuple.size())
		return NULL;

	PyChameleon * itr = mTuple[index];
	if (itr == NULL)
		return NULL;
	PyObject* obj = itr->getPyObject();
	if (obj == NULL || obj->gettype() != PyTypeString)
    {
        printf("object isn't a string\n");
		return NULL;
    }

	return (PyString*)obj;
}

PySubStream* PyTuple::GetItem_asPySubStream( const int index )
{
	if (index < 0)
		return NULL;
	if (index > (int)mTuple.size())
		return NULL;

	PyChameleon * itr = mTuple[index];
	if (itr == NULL)
		return NULL;
	PyObject* obj = itr->getPyObject();
	if (obj == NULL || obj->gettype() != PyTypeSubStream)
		return NULL;

	return (PySubStream*)obj;
}

PyClass* PyTuple::GetItem_asPyClass( const int index )
{
    if (index < 0)
        return NULL;
    if (index > (int)mTuple.size())
        return NULL;

    PyChameleon * itr = mTuple[index];
    if (itr == NULL)
        return NULL;
    PyObject* obj = itr->getPyObject();
    if (obj == NULL || obj->gettype() != PyTypeClass)
        return NULL;

    return (PyClass*)obj;
}

bool PyTuple::set_item( const int index, PyObject *object )
{
	if (index < 0)
		return false;
	if (index+1 > (int)mTuple.size())
		mTuple.resize(index+1);

    // we'r storing the object somewhere else so... dec the ref..
    PyIncRef(object);

	PyChameleon & itr = *mTuple[index];
	itr.setPyObject(object);
	return true;
}

/************************************************************************/
/* PyList                                                               */
/************************************************************************/
PyList::PyList() : PyObject( PyTypeList ) {}
PyList::PyList( size_t count ) : PyObject( PyTypeList )
{
	mList.resize( count );
}

PyList::~PyList()
{
    clear();	
}

size_t PyList::size()
{
	return mList.size();
}

bool PyList::add( PyObject* obj )
{
	if (obj == NULL)
		return false;

    /* because we store a extra ref of the object we should increase the ref counter */
	mList.push_back(obj);
    PyIncRef(obj);

	return true;
}

uint32 PyList::hash()
{
    uint32 hsh = 0;
    PyList::iterator itr = mList.begin();
    for (; itr != mList.end(); itr++)
    {
        PyObject* obj = (*itr);
        assert(obj);
        hsh|= PyObject_Hash(obj);
        hsh = hsh << 3;
    }
    return hsh;
}

PyList::iterator PyList::begin()
{
	return mList.begin();
}

PyList::iterator PyList::end()
{
	return mList.end();
}

/* NEVER EVER USE THIS FOR REGULAR USE BECAUSE OF THE REF STEALING */
bool PyList::init( PyObject* list )
{
    if (list == NULL)
        return false;

    if (!PyTuple_Check(list))
        return false;

    PyList* plist = (PyList*)list;

    /* if we are already filled, clear us */
    if (size() != 0)
        clear();

    /* we're gonna steal references here... */
    size_t index = 0;
    for (PyList::iterator itr = plist->begin(); itr != plist->end(); itr++) {
        set_item(index++, *itr);
    }

    return true;
}

bool PyList::resize( size_t new_size )
{
    if (new_size == mList.size())
        return true;

    /* resize when the requested size is larger than the current size */
    if (new_size > mList.size()) {
        mList.resize(new_size);
    } else {
    /* very ugly situation: if we resize to a size that is smaller than the current; we need to decref the
     * objects that are "disapearing" because of the resize.
     */
        size_t new_end_index = new_size - mList.size();

        if (new_end_index == 0)
            return true;

        if (new_end_index < 0) // this should never happen.
            return false;

        iterator Itr = mList.end();
        while (new_end_index--) {
            PyDecRef( *Itr );
            mList.erase(Itr);
            Itr = mList.end();
        }
    }
    return true;
}

bool PyList::clear()
{
    iterator Itr = mList.begin();
    for (; Itr != mList.end(); Itr++)
        PyDecRef( *Itr );

    mList.clear();
    return true;
}

bool PyList::set_item( size_t index, PyObject* obj )
{
    /* we have to check if we already have a object at the index position */
    PyObject* tobj = mList[index];

    if (tobj != NULL)
        PyDecRef(tobj);

    mList[index] = obj;
    return true;
}

PyObject* PyList::get_item( size_t index )
{
    /* return NULL because I don't want to  */
    if (index > mList.size())
        return NULL;

    return mList[index];
}

/************************************************************************/
/* PyDict                                                               */
/************************************************************************/
PyDict::PyDict() : PyObject(PyTypeDict), mMappingMode(true), mMappingIndex(0) {}

PyDict::~PyDict()
{
    int i = 0;
    iterator itr = mDict.begin();
    for (; itr != mDict.end(); itr++)
    {
        PyDictEntry * entry = itr->second;
        assert(entry);
        assert(entry->key);
        assert(entry->obj);

        entry->key->DecRef();
        entry->obj->DecRef();
        SafeFree(entry);
        i++;
    }

    mDict.clear();
}

// this one needs to die...
PyChameleon PyDict::operator[]( const char* keyName )
{
    ASCENT_HARDWARE_BREAKPOINT;
    if (keyName == NULL || *keyName == '\0')
        return PyErrorIterator;

    uint32 hsh = Utils::Hash::sdbm_hash(keyName);

    PyDictEntry * entry = mDict[hsh];
    if (entry == NULL)
    {
        entry = (PyDictEntry*)malloc(sizeof( PyDictEntry ));//new PyDictEntry;
        entry->key = (PyObject*)new PyString(keyName);
        entry->obj = NULL;
        mDict[hsh] = entry;
    }

    return entry;
}

size_t PyDict::size()
{
    return mDict.size();
}

// fix mapping mode.... to update on get_item..
bool PyDict::set_item( PyObject* key, PyObject* obj )
{
    if (key == NULL || obj == NULL)
    {
        ASCENT_HARDWARE_BREAKPOINT;
        return false;
    }

    /*if (key->gettype() == PyTypeNone || obj->gettype() == PyTypeNone)
    {
        ASCENT_HARDWARE_BREAKPOINT;
        return false;
    }*/

    /*if (mMappingMode == true)
    {
        // create a new dictionary entry 
        PyDictEntry * entry = new PyDictEntry;
        entry->key = key;
        entry->obj = obj;
        mDict.insert(std::make_pair(mMappingIndex++, entry));
        key->IncRef();  // we seem to reuse a object that is already in the system so increase its mojo
        obj->IncRef();
    }
    else*/
    {
        // hit debug as I want to make sure its working correctly.
        //ASCENT_HARDWARE_BREAKPOINT;

        uint32 hsh = PyObject_Hash(key);

        iterator itr = mDict.find(hsh);

        /* check if we are updating a dictionary entry or creating a new one */
        if (itr == mDict.end())
        {
            /* create a new dictionary entry */
            //PyDictEntry * entry = new PyDictEntry();
            PyDictEntry * entry = (PyDictEntry*)malloc(sizeof( PyDictEntry ));
            entry->key = key;
            entry->obj = obj;
            mDict.insert(std::make_pair(hsh, entry));
            PyIncRef(key); // we seem to reuse a object that is already in the system so increase its mojo
            PyIncRef(obj);
        }
        else
        {
            /* update/replace a already existing entry ( bit tricky ) */
            ASCENT_HARDWARE_BREAKPOINT;
            PyDictEntry * entry = itr->second;
            PyDecRef(entry->obj);
            entry->obj = obj;
            PyIncRef(obj);
        }
    }

    return true;
}

PyObject* PyDict::get_item( PyObject* key )
{
    ASCENT_HARDWARE_BREAKPOINT;
    if (key == NULL)
        return NULL;
    return NULL;
}

PyObject* PyDict::get_item(const char* key_name)
{
    //ASCENT_HARDWARE_BREAKPOINT;
    if (key_name == NULL || *key_name == '\0')
        return NULL;

    uint32 hsh = Utils::Hash::sdbm_hash(key_name);

    PyDictEntry * entry = mDict[hsh];
    if (entry == NULL)
    {
        ASCENT_HARDWARE_BREAKPOINT;
        entry = (PyDictEntry*)malloc(sizeof( PyDictEntry ));//new PyDictEntry;
        entry->key = (PyObject*)new PyString(key_name);
        entry->obj = NULL;
        mDict[hsh] = entry;
    }

    return entry->obj;
}

PyObject* PyDict::get_item(const char* key_name, PyObject* default_obj)
{
    //ASCENT_HARDWARE_BREAKPOINT;
    if (key_name == NULL || *key_name == '\0')
    {
        default_obj->DecRef();
        return NULL;
    }

    uint32 hsh = Utils::Hash::sdbm_hash(key_name);

    PyDictEntry * entry = mDict[hsh];
    if (entry == NULL)
    {
        //ASCENT_HARDWARE_BREAKPOINT;
        entry = (PyDictEntry*)malloc(sizeof( PyDictEntry ));//new PyDictEntry();
        entry->key = (PyObject*)new PyString(key_name);
        entry->obj = default_obj; PyIncRef(default_obj);
        mDict[hsh] = entry;
    }

    // another wierd exception
    if (entry->obj == NULL)
        ASCENT_HARDWARE_BREAKPOINT;

    PyDecRef(default_obj);
    return entry->obj;
}

uint32 PyDict::hash()
{
    uint32 hsh = 0;
    iterator itr = mDict.begin();
    for (; itr != mDict.end(); itr++)
    {
        PyDictEntry * entry = itr->second;
        hsh|= PyObject_Hash(entry->key);
        hsh = hsh << 3;
        hsh|= PyObject_Hash(entry->obj);
    }

    return hsh;
}

PyDict::iterator PyDict::begin()
{
    return mDict.begin();
}

PyDict::iterator PyDict::end()
{
    return mDict.end();
}

bool PyDict::scanf( const char * keyName, const char* format, ... )
{
    /* useless call.. because the dict need a lookup callback for this.... lol I actualy don't care.*/
    update();

    uint32 hsh = Utils::Hash::sdbm_hash(keyName);

    PyDict::iterator fItr = mDict.find(hsh);
    if (fItr == mDict.end())
        return false;

    PyObject * foundObject = ((PyDictEntry *)fItr->second)->obj;

    va_list ap;
    va_start(ap, format);

    //sum += i;
    //count++;
    //i = va_arg( ap, (void*));

    void* pVar = NULL;
    int formatIndex = 0;
    //while (pVar != -1)
    //{
    pVar = va_arg( ap, void*);

    char tag = format[formatIndex];
    switch(tag)
    {
        /*unicode string*/
    case 'u':
        std::wstring * str = (std::wstring *)pVar;
        str->clear();

        size_t len = ((PyUnicodeUCS2*)foundObject)->length();
        wchar_t * buff = ((PyUnicodeUCS2*)foundObject)->content();
        str->append(buff, len);
        break;
    }

    //formatIndex++;
    //}

    va_end(ap);
    return true;
}

void PyDict::update()
{
    if(mMappingMode == true)
    {
        //ASCENT_HARDWARE_BREAKPOINT;
        PyDictEntry * entry = NULL;
        iterator itr = mDict.begin();
        iterator dItr;

        size_t dictSize = mDict.size();
        for (size_t i = 0; i < dictSize; ++i)
        {
            entry = itr->second;

            uint32 hsh = PyObject_Hash(entry->key);
            dItr = itr++;

            mDict.erase(dItr);
            mDict.insert(mDict.end(), std::make_pair(hsh, entry));
        }
        mMappingMode = false;
    }
}

bool PyDict::get_buffer( const char * keyName, char* dst, size_t dst_len )
{
    if (keyName == NULL || dst == NULL || dst_len == 0)
        return false;

    update();

    uint32 hsh = Utils::Hash::sdbm_hash(keyName);

    PyDict::iterator fItr = mDict.find(hsh);
    if (fItr == mDict.end())
        return false;

    PyObject * foundObject = ((PyDictEntry *)fItr->second)->obj;

    if (foundObject == NULL)
        return false;

    if (foundObject->gettype() != PyTypeString)
        return false;

    PyString * str = (PyString *)foundObject;

    /* check if we have enough room to read the string / buffer */
    if (str->length() > dst_len)
        return false;

    ASCENT_MEMCPY(dst, str->content(), str->length());
    return true;
}

bool PyDict::set_int( const char * keyName, int number )
{
    return set_item(keyName, (PyObject *)new PyInt(number));
}

bool PyDict::set_double( const char * keyName, double number )
{
    return set_item(keyName, (PyObject *)new PyFloat(number));
}

bool PyDict::set_str( const char * keyName, const char* str )
{
    return set_item(keyName, (PyObject *)new PyString(str));
}

bool PyDict::set_bool( const char * keyName, bool check )
{
    return set_item(keyName, (PyObject *)new PyBool(check));
}

/************************************************************************/
/* PySubStream                                                          */
/************************************************************************/
PySubStream::PySubStream() : PyObject(PyTypeSubStream), mData(NULL), mLen(0) {}

PySubStream::PySubStream( uint8* data, size_t len ) : PyObject(PyTypeSubStream), mData(NULL), mLen(0)
{
	if (data == NULL || len == 0)
		return;

	mLen = len;
	mData = ASCENT_MALLOC(mLen+1);
	ASCENT_MEMCPY(mData, data, mLen);
}

PySubStream::~PySubStream()
{
	SafeFree(mData);

	mLen = 0;
}

uint8* PySubStream::content()
{
	return (uint8*)mData;
}

bool PySubStream::set( uint8 * data, size_t len )
{
	if (mData != NULL)
		mData = ASCENT_REALLOC(mData, len+1);
	else
		mData = ASCENT_MALLOC(len+1);
	
	if (mData == NULL)
	{
		mLen = 0;
		return false;
	}

	mLen = len;

	ASCENT_MEMCPY(mData, data, mLen);
	return true;
}


size_t PySubStream::size()
{
	return mLen;
}

uint32 PySubStream::hash()
{
	return Utils::Hash::sdbm_hash((char*)mData, (int)mLen);
}

/************************************************************************/
/* PyClass                                                              */
/************************************************************************/
PyClass::PyClass( const char* class_name ) : PyObject( PyTypeClass ), mDict( new PyDict() ), mName( new PyString( class_name ) ), mBases(NULL), mWeakRefList(NULL), mInDict(NULL)
{
}

PyClass::~PyClass()
{
    destruct();

	if (mName)
	{
		mName->DecRef();
		mName = NULL;
	}
	if (mBases)
	{
		mBases->DecRef();
		mBases = NULL;
	}
	
	if (mDict)
	{
		mDict->DecRef();
		mDict = NULL;
	}
	
	if (mWeakRefList)
	{
		mWeakRefList->DecRef();
		mWeakRefList = NULL;
	}

	if (mInDict)
	{
		mInDict->DecRef();
		mInDict = NULL;
	}
}

bool PyClass::setbases( PyTuple* tuple )
{
	ASCENT_ASSERT(mBases == NULL);

	// object should exist
	if (tuple == NULL)
		return false;

	mBases = tuple;
	return true;
}

bool PyClass::setdict( PyDict* dict )
{
	ASCENT_ASSERT(mDict == NULL);

	// object should exist
	if (dict == NULL)
		return false;

	if (dict->gettype() != PyTypeDict)
		return false;

	mDict = dict;
	return true;
}

bool PyClass::setDirList( PyList * list )
{
	ASCENT_ASSERT(mWeakRefList == NULL);
	//assert(list->gettype() == PyTypeList);
	if (list == NULL)
		return false;
	mWeakRefList = list;
	return true;
}

bool PyClass::setDirDict( PyDict * dict )
{
	ASCENT_ASSERT(mInDict == NULL);
	if (dict == NULL)
		return false;
	mInDict = dict;
	return true;
}

PyString* PyClass::getname()
{
	return mName;
}

PyTuple* PyClass::getbases()
{
	return mBases;
}

PyDict* PyClass::getdict()
{
	return mDict;
}

PyList * PyClass::getDirList()
{
	return mWeakRefList;
}

PyDict * PyClass::getDirDict()
{
	return mInDict;
}

/**
 * @note this hash is fucked up.
 */
uint32 PyClass::hash()
{
	uint32 hsh = 0;
	if (mName)
	{
		hsh = mName->hash();
		hsh = hsh << 3;
	}
	
	if (mBases)
	{
		hsh|= mBases->hash();
		hsh = hsh << (mBases->size() / 3);
	}

	if (mDict)
	{
		hsh|= mDict->hash();
		hsh = hsh << (mDict->size() / 3);
	}
	
	if (mWeakRefList)
	{
		hsh|= mWeakRefList->hash();
		hsh = hsh << (mWeakRefList->size() / 10);
	}

	if (mInDict)
	{
		hsh|= mInDict->hash();
		hsh = hsh << (mInDict->size() / 10);
	}

	return hsh;
}

void PyObject::IncRef()
{
    mRefcnt++;
}

void PyObject::DecRef()
{
    mRefcnt--;
    if (mRefcnt <= 0)
        delete this;
}

uint8 PyObject::gettype()
{
    return mType;
}

PyObject::~PyObject()
{
}

PyObject::PyObject(PyType type) : mType(type), mRefcnt(1) {}

size_t PyObject::GetRef()
{
    return mRefcnt;
}

createFileSingleton( PyBaseNone );

PyBaseNone::PyBaseNone() : PyObject(PyTypeNone) {}

// hash a none object? your seriouse?
uint32 PyBaseNone::hash()
{
    ASCENT_HARDWARE_BREAKPOINT;
	return Utils::Hash::sdbm_hash("PyNone");
}

/************************************************************************/
/* PyPackedRow                                                          */
/************************************************************************/
PyPackedRow::PyPackedRow() : PyObject(PyTypePackedRow), mHeader(NULL), mRawFieldData(NULL), mRawFieldDataLen(0) {}

PyPackedRow::~PyPackedRow()
{
	iterator Itr = flowers.begin();
	for (; Itr != flowers.end(); Itr++)
	{
		delete *Itr;
	}

	flowers.clear();

	if (mHeader)
		mHeader->DecRef();

	if (rawPayLoad)
		rawPayLoad->DecRef();

	if (mRawFieldData)
		SafeFree(mRawFieldData);
}

bool PyPackedRow::addleaf( PyObject* leaf )
{
	assert(leaf != NULL);
	
	// fucked up because of some c++ fuckup...
	uint32 hsh = PyObject_Hash(leaf);
	PyChameleon * itr = new PyChameleon();
	itr->setPyObject(leaf);

	flowers.push_back(itr);
	
	return true;
}

uint32 PyPackedRow::hash()
{
	uint32 hsh = 0;

	if (mHeader)
		hsh |= PyObject_Hash((PyObject *)mHeader);

	if (rawPayLoad)
	{
		hsh = hsh << 3;
		hsh |= PyObject_Hash((PyObject *)mHeader);
	}

	iterator itr = flowers.begin();
	for (; itr != flowers.end(); itr++)
	{
		hsh = hsh << 3;
		if((*itr)->getPyObject())
			hsh |= PyObject_Hash((*itr)->getPyObject());
	}
	return hsh;
}

bool PyPackedRow::setheader( PyClass * obj )
{
	if (obj->gettype() != PyTypeClass)
		return false;

	mHeader = obj;
	return true;
}

PyPackedRow::iterator PyPackedRow::begin()
{
	return flowers.begin();
}

PyPackedRow::iterator PyPackedRow::end()
{
	return flowers.end();
}

PyClass* PyPackedRow::getheader()
{
	return mHeader;
}

size_t PyPackedRow::size()
{
	return flowers.size();
}

bool PyPackedRow::setRawPayLoad( PyObject* tuple )
{
	rawPayLoad = (PyTuple*)tuple;
	return true;
}

PyObject* PyPackedRow::getRawPayLoad()
{
	return (PyObject*)rawPayLoad;
}

bool PyPackedRow::init( PyObject* header )
{
	size_t argumentCount = 0;
	if (header->gettype() == PyTypeTuple)
	{
		PyTuple& tulple = *(PyTuple*)header;
		argumentCount = tulple.size();
		if (argumentCount == 0)
		{
			Log.Error("PyPackedRow", "init: expected at least one argument");
			return false;
		}
	}
	else
	{
		argumentCount = 1; // I have no clue
	}

	//mHeader = header;

	return true;
}

PyObject* PyPackedRow::GetFieldObject( int index )
{
	PyTuple & fieldHelper = *rawPayLoad;
	PyObject * res = fieldHelper[index].getPyObject();
	//f ()
	return 0;
}

PyObject* PyPackedRow::GetLeaf( int i )
{
	return flowers[i]->getPyObject();
}

/************************************************************************/
/* substruct                                                            */
/************************************************************************/
PySubStruct::PySubStruct() : PyObject(PyTypeSubStruct), payload(NULL) {}

PySubStruct::~PySubStruct()
{
	if (payload)
		payload->DecRef();
}

PyObject * PySubStruct::getPyObject()
{
	return payload;
}

bool PySubStruct::setPyObject( PyObject* obj )
{
	if (obj == NULL)
		return false;

	payload = obj;
	PyIncRef(obj);

	return true;
}

// class related hash function
uint32 PySubStruct::hash()
{
	ASCENT_HARDWARE_BREAKPOINT;
}

/************************************************************************/
/* PyModule                                                             */
/************************************************************************/
PyModule::PyModule() : PyObject(PyTypeModule), mModuleName(NULL) {}

PyModule::~PyModule() {}


uint32 PyModule::hash()
{
	ASCENT_HARDWARE_BREAKPOINT;
    return 0;
}

uint32 PyObject_Hash( PyObject* obj )
{
	return obj->hash();
}

// this is so wrong 'callable_object' should only contain the method to call...
// now we are assuming they always call init and new...
PyObject * PyObject_CallObject( PyObject *callable_object, PyObject *args )
{
    assert(callable_object);
    assert(args);
    PyClass * pClass = (PyClass *)callable_object;
    PyClass * pNewClass = pClass->New();
    pNewClass->init(args);
    return pNewClass;
}
