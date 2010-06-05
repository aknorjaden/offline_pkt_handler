#include "PyObjectDumper.h"

#pragma pack(push,1)

class dbutil_CRowset : public PyClass
{
protected:
public:
    dbutil_CRowset() : PyClass()
    {
        setname(new PyString("dbutil.CRowset"));
        mList = new PyList();
    }
    ~dbutil_CRowset()
    {
        mList->DecRef();
    };

    dbutil_CRowset* New()
    {
        return new dbutil_CRowset();
    }

    //list.__init__(self, rows)
    //self.header = header

    // comments: format guessed from compiled scripts
    bool init(PyObject* state)
    {
        /*if (state->gettype() != PyTypeTuple)
        return false;
        PyTuple * pTuple = (PyTuple*)state;

        mDict->set_item("header", pTuple->GetItem(0));
        mDict->set_item("lines", pTuple->GetItem(1));
        mDict->set_item("RowClass", pTuple->GetItem(2));*/

        if (state->gettype() != PyTypeDict)
            return false;

        // TODO check if this has any data in it... it should be empty
        mDict->DecRef();

        // replace the current dict with that one to init...
        mDict = (PyDict*)state; state->IncRef();

        return true;
    }

    PyTuple* GetState()
    {
        return NULL;
    };

    // this object derives from a PyList.... we hack it this way for now...
    PyList * mList;
};

#pragma pack(pop)