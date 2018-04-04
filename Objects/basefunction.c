/* base_function object implementation */

#include "Python.h"
#include "internal/mem.h"
#include "internal/pystate.h"
#include "structmember.h"

PyObject *
PyBaseFunction_New(PyTypeObject *cls, PyCFunctionDef *ml,
                   PyObject *self, PyObject *module,
                   PyObject *parent)
{
    PyBaseFunctionObject *op;
    op = PyObject_GC_New(PyBaseFunctionObject, cls);
    if (op == NULL)
        return NULL;

    _PyObject_GC_TRACK(op);

    op->m_ml = ml;
    Py_XINCREF(self);
    op->m_self = self;
    Py_XINCREF(module);
    op->m_module = module;
    Py_XINCREF(parent);
    op->m_parent = parent;

    /* Zero all other bytes, starting with m_weakreflist.
       This is for the convenience of subclasses. */
    const size_t off = offsetof(PyBaseFunctionObject, m_weakreflist);
    memset((char *)op + off, 0, cls->tp_basicsize - off);

    return (PyObject *)op;
}


PyCFunction
PyBaseFunction_GetFunction(PyObject *op)
{
    if (!PyBaseFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyBaseFunction_GET_FUNCTION(op);
}


PyObject *
PyBaseFunction_GetSelf(PyObject *op)
{
    if (!PyBaseFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyBaseFunction_GET_SELF(op);
}


int
PyBaseFunction_GetFlags(PyObject *op)
{
    if (!PyBaseFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    return PyBaseFunction_GET_FLAGS(op);
}


/* Methods */

PyDoc_STRVAR(func_doc,
"base_function is the base class for all function classes.\n"
"\n"
"base_function cannot be instantiated.");

int
_PyBaseFunction_Clear(PyObject *m)
{
    PyBaseFunctionObject* func = (PyBaseFunctionObject *)m;
    Py_CLEAR(func->m_parent);
    Py_CLEAR(func->m_module);
    Py_CLEAR(func->m_self);
    return 0;
}


static void
func_dealloc(PyBaseFunctionObject *m)
{
    _PyObject_GC_UNTRACK(m);
    _PyBaseFunction_Clear((PyObject *)m);
    if (m->m_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)m);
    }
    Py_TYPE(m)->tp_free(m);
}


static int
func_traverse(PyBaseFunctionObject *m, visitproc visit, void *arg)
{
    Py_VISIT(m->m_self);
    Py_VISIT(m->m_module);
    Py_VISIT(m->m_parent);
    return 0;
}


static PyObject *
func_reduce(PyBaseFunctionObject *m)
{
    PyObject *parent = m->m_self;
    if (parent == NULL || parent == Py_None || PyModule_Check(parent)) {
        parent = m->m_parent;
        if (parent == NULL || PyModule_Check(parent)) {
            return PyUnicode_FromString(m->m_ml->ml_name);
        }
    }

    _Py_IDENTIFIER(getattr);
    PyObject *builtins = PyEval_GetBuiltins();
    PyObject *getattr = _PyDict_GetItemId(builtins, &PyId_getattr);
    return Py_BuildValue("O(Os)", getattr, parent, m->m_ml->ml_name);
}


static PyMethodDef func_methods[] = {
    {"__reduce__", (PyCFunction)func_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};


static PyObject *
func_get__name__(PyBaseFunctionObject *m, void *closure)
{
    return PyUnicode_FromString(m->m_ml->ml_name);
}


static PyObject *
func_get__qualname__(PyBaseFunctionObject *m, void *closure)
{
    /* If __parent__ is set and not a module, then
       return m.__parent__.__qualname__+ '.' + m.__name__:

       >>> dict.get.__qualname__
       'dict.get'
       >>> {}.get.__qualname__
       'dict.get'

       Otherwise, just return m.__name__:

       >>> len.__qualname__
       'len'
    */

    PyObject *parent = (PyObject*)m->m_parent;

    if (parent == NULL || PyModule_Check(parent)) {
        return PyUnicode_FromString(m->m_ml->ml_name);
    }

    _Py_IDENTIFIER(__qualname__);
    PyObject *parent_qualname = _PyObject_GetAttrId(parent, &PyId___qualname__);
    if (parent_qualname == NULL)
        return NULL;

    if (!PyUnicode_Check(parent_qualname)) {
        PyErr_SetString(PyExc_TypeError, "<base_function>.__parent__."
                        "__qualname__ is not a unicode object");
        Py_DECREF(parent_qualname);
        return NULL;
    }

    PyObject *res = PyUnicode_FromFormat("%S.%s", parent_qualname, m->m_ml->ml_name);
    Py_DECREF(parent_qualname);
    return res;
}


static PyObject *
func_get__self__(PyObject *m, void *closure)
{
    PyObject *self = PyBaseFunction_GET_SELF(m);

    if (self == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "'%.200s' object has no attribute '__self__'",
                     Py_TYPE(m)->tp_name);
        return NULL;
    }
    Py_INCREF(self);
    return self;
}


static PyObject *
func_get__objclass__(PyObject *m, void *closure)
{
    PyObject *objclass = (PyObject *)PyBaseFunction_GET_OBJCLASS(m);

    if (objclass == NULL) {
        PyErr_Format(PyExc_AttributeError,
                     "'%.200s' object has no attribute '__objclass__'",
                     Py_TYPE(m)->tp_name);
        return NULL;
    }
    Py_INCREF(objclass);
    return objclass;
}


static PyGetSetDef func_getsets [] = {
    {"__name__", (getter)func_get__name__, NULL, NULL},
    {"__qualname__", (getter)func_get__qualname__, NULL, NULL},
    {"__self__", (getter)func_get__self__, NULL, NULL},
    {"__objclass__", (getter)func_get__objclass__, NULL, NULL},
    {NULL}
};


#define OFF(x) offsetof(PyBaseFunctionObject, x)

static PyMemberDef func_members[] = {
    {"__module__",  T_OBJECT,     OFF(m_module), PY_WRITE_RESTRICTED},
    {"__parent__",  T_OBJECT_EX,  OFF(m_parent), READONLY},
    {NULL}
};


static PyObject *
func_repr(PyBaseFunctionObject *m)
{
    return PyUnicode_FromFormat("<%s %s>",
                                Py_TYPE(m)->tp_name,
                                m->m_ml->ml_name);
}


/* Compare two objects, one of them being a base_function instance.
   op must be Py_EQ or Py_NE. */
int
_PyBaseFunction_RichCompareBool(PyObject *self, PyObject *other, int op)
{
    if (Py_TYPE(self) != Py_TYPE(other)) {
        return (op != Py_EQ);
    }

    /* Equal types => both objects are base_function instances */
    PyBaseFunctionObject *a = (PyBaseFunctionObject *)self;
    PyBaseFunctionObject *b = (PyBaseFunctionObject *)other;
    if (a->m_ml != b->m_ml) {
        return (op != Py_EQ);
    }
    return PyObject_RichCompareBool(PyBaseFunction_GET_SELF(a),
                                    PyBaseFunction_GET_SELF(b), op);
}

static PyObject *
func_richcompare(PyObject *self, PyObject *other, int op)
{
    int b;
    if ((op != Py_EQ && op != Py_NE) || !PyBaseFunction_CheckFast(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (self == other) {
        b = (op == Py_EQ);
    }
    else {
        b = _PyBaseFunction_RichCompareBool(self, other, op);
        if (b == -1) {
            return NULL;
        }
    }
    PyObject *res = b ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
}


static Py_hash_t
func_hash(PyBaseFunctionObject *m)
{
    Py_uhash_t mult = _PyHASH_MULTIPLIER, err = -1;
    Py_uhash_t h = 0, t;

    if (m->m_self != NULL) {
        h = PyObject_Hash(m->m_self);
        if (h == err) {
            return -1;
        }
    }

    t = _Py_HashPointer(Py_TYPE(m));
    if (t == err) {
        return -1;
    }
    h = (h * mult) + t;

    t = _Py_HashPointer(m->m_ml->ml_meth);
    if (t == err) {
        return -1;
    }
    h = (h * mult) + t;

    if (h == err) {
        h--;
    }
    return h;
}


static PyObject *
func_get(PyObject *func, PyObject *obj, PyObject *type)
{
    /* Check cases where we just return the function as-is */
    if (obj == NULL ||                           /* Binding to a class */
        PyBaseFunction_REAL_SELF(func) != NULL)  /* Already bound */
    {
        Py_INCREF(func);
        return func;
    }

    return PyMethod_New(func, obj);
}


PyTypeObject PyBaseFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "base_function",
    sizeof(PyBaseFunctionObject),
    0,
    (destructor)func_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_as_async */
    (reprfunc)func_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)func_hash,                        /* tp_hash */
    PyBaseFunction_Call,                        /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_BASEFUNCTION,                /* tp_flags */
    func_doc,                                   /* tp_doc */
    (traverseproc)func_traverse,                /* tp_traverse */
    _PyBaseFunction_Clear,                      /* tp_clear */
    (richcmpfunc)func_richcompare,              /* tp_richcompare */
    OFF(m_weakreflist),                         /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    func_methods,                               /* tp_methods */
    func_members,                               /* tp_members */
    func_getsets,                               /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    (descrgetfunc)func_get,                     /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    0,                                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};
