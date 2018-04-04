/* bound_method object implementation */

#include "Python.h"
#include "internal/mem.h"
#include "internal/pystate.h"
#include "structmember.h"

#define TP_DESCR_GET(t) ((t)->tp_descr_get)

/* Free list for method objects to safe malloc/free overhead */
static PyMethodObject *free_list;
#define next_free base.m_self
static int numfree = 0;
#ifndef PyMethod_MAXFREELIST
#define PyMethod_MAXFREELIST 256
#endif

_Py_IDENTIFIER(__name__);
_Py_IDENTIFIER(__qualname__);

PyObject *
PyMethod_Function(PyObject *im)
{
    if (!PyMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyMethodObject *)im)->im_func;
}

/* Method objects are used for bound instance methods returned by
   instancename.methodname. ClassName.methodname returns an ordinary
   function.
*/


/* TODO: use METH_FASTCALL here */
static PyCFunctionDef ml_method_indirect_call =
    {"bound_method", (PyCFunction)_PyObject_Call_Prepend,
            METH_VARARGS | METH_KEYWORDS | METH_PASS_FUNCTION | METH_CALL_UNBOUND};


PyObject *
PyMethod_New(PyObject *func, PyObject *self)
{
    PyMethodObject *im;
    if (func == NULL || self == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    int do_bind = 0;
    PyObject *parent = NULL;

    if (PyBaseFunction_CheckFast(func)) {
        if (PyBaseFunction_REAL_SELF(func) == NULL) {
            do_bind = 1;
            if (_PyBaseFunction_CheckObjclass(func, self) < 0) {
                return NULL;
            }
        }
        parent = PyBaseFunction_GET_PARENT(func);
    }

    /* Create new method from free list if possible */
    im = free_list;
    if (im != NULL) {
        free_list = (PyMethodObject *)(im->next_free);
        (void)PyObject_INIT(im, &PyMethod_Type);
        numfree--;
    }
    else {
        im = (PyMethodObject*)PyObject_GC_New(PyMethodObject, &PyMethod_Type);
        if (im == NULL)
            return NULL;
    }

    Py_INCREF(self);
    im->base.m_self = self;
    /* m_module is not needed because we override __module__ */
    im->base.m_module = NULL;
    Py_XINCREF(parent);
    im->base.m_parent = parent;
    im->base.m_weakreflist = NULL;
    Py_INCREF(func);
    im->im_func = func;

    if (do_bind && !PyBaseFunction_HAS_FLAG(func, METH_CALL_UNBOUND)) {
        /* Re-use the m_ml from the original function */
        PyBaseFunctionObject *f = (PyBaseFunctionObject *)func;
        im->base.m_ml = f->m_ml;
    }
    else {
        /* General case */
        im->base.m_ml = &ml_method_indirect_call;
    }

    _PyObject_GC_TRACK(im);
    return (PyObject *)im;
}


static PyObject *
method_reduce(PyMethodObject *im, PyObject *Py_UNUSED(ignored))
{
    PyObject *self = PyMethod_GET_SELF(im);
    PyObject *func = PyMethod_GET_FUNCTION(im);
    PyObject *builtins;
    PyObject *getattr;
    PyObject *funcname;
    _Py_IDENTIFIER(getattr);

    funcname = _PyObject_GetAttrId(func, &PyId___name__);
    if (funcname == NULL) {
        return NULL;
    }
    builtins = PyEval_GetBuiltins();
    getattr = _PyDict_GetItemId(builtins, &PyId_getattr);
    return Py_BuildValue("O(ON)", getattr, self, funcname);
}


static PyMethodDef method_methods[] = {
    {"__reduce__", (PyCFunction)method_reduce, METH_NOARGS, NULL},
    {NULL, NULL}
};

/* Descriptors for PyMethod attributes */

#define MO_OFF(x) offsetof(PyMethodObject, x)

static PyMemberDef method_memberlist[] = {
    {"__func__", T_OBJECT, MO_OFF(im_func), READONLY|RESTRICTED,
     "the function (or other callable) implementing a method"},
    {NULL}      /* Sentinel */
};


/* Override __doc__, __name__, __qualname__ and __module__
   to go through __func__ instead. */
static PyObject *
method_get_doc(PyObject *im, void *context)
{
    _Py_IDENTIFIER(__doc__);
    return _PyObject_GetAttrId(PyMethod_GET_FUNCTION(im), &PyId___doc__);
}

static PyObject *
method_get_name(PyObject *im, void *context)
{
    _Py_IDENTIFIER(__name__);
    return _PyObject_GetAttrId(PyMethod_GET_FUNCTION(im), &PyId___name__);
}

static PyObject *
method_get_qualname(PyObject *im, void *context)
{
    _Py_IDENTIFIER(__qualname__);
    return _PyObject_GetAttrId(PyMethod_GET_FUNCTION(im), &PyId___qualname__);
}

static PyObject *
method_get_module(PyObject *im, void *context)
{
    _Py_IDENTIFIER(__module__);
    return _PyObject_GetAttrId(PyMethod_GET_FUNCTION(im), &PyId___module__);
}


static PyGetSetDef method_getset[] = {
    {"__doc__", method_get_doc, NULL, NULL},
    {"__name__", method_get_name, NULL, NULL},
    {"__qualname__", method_get_qualname, NULL, NULL},
    {"__module__", method_get_module, NULL, NULL},
    {0}
};

static PyObject *
method_getattro(PyObject *obj, PyObject *name)
{
    PyMethodObject *im = (PyMethodObject *)obj;
    PyTypeObject *tp = obj->ob_type;
    PyObject *descr = NULL;

    {
        if (tp->tp_dict == NULL) {
            if (PyType_Ready(tp) < 0)
                return NULL;
        }
        descr = _PyType_Lookup(tp, name);
    }

    if (descr != NULL) {
        descrgetfunc f = TP_DESCR_GET(descr->ob_type);
        if (f != NULL)
            return f(descr, obj, (PyObject *)obj->ob_type);
        else {
            Py_INCREF(descr);
            return descr;
        }
    }

    return PyObject_GetAttr(im->im_func, name);
}

PyDoc_STRVAR(method_doc,
"method(function, instance)\n\
\n\
Create a bound method object.");

static PyObject *
method_new(PyTypeObject* type, PyObject* args, PyObject *kw)
{
    PyObject *func;
    PyObject *self;

    if (!_PyArg_NoKeywords("method", kw))
        return NULL;
    if (!PyArg_UnpackTuple(args, "method", 2, 2,
                          &func, &self))
        return NULL;
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "first argument must be callable");
        return NULL;
    }
    if (self == NULL || self == Py_None) {
        PyErr_SetString(PyExc_TypeError,
            "self must not be None");
        return NULL;
    }

    return PyMethod_New(func, self);
}


static void
method_dealloc(PyMethodObject *im)
{
    _PyObject_GC_UNTRACK(im);
    if (im->base.m_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)im);
    }
    _PyBaseFunction_Clear((PyObject *)im);
    Py_DECREF(im->im_func);
    if (numfree < PyMethod_MAXFREELIST) {
        im->next_free = (PyObject *)free_list;
        free_list = im;
        numfree++;
        return;
    }
    PyObject_GC_Del(im);
}


static PyObject *
method_richcompare(PyObject *self, PyObject *other, int op)
{
    int b;
    if ((op != Py_EQ && op != Py_NE) || !PyBaseFunction_CheckFast(other)) {
        Py_RETURN_NOTIMPLEMENTED;
    }
    if (self == other) {
        b = (op == Py_EQ);
    }
    else {
        b = _PyBaseFunction_RichCompareBool(self, other, op);
        if (b == (op == Py_EQ)) {
            b = PyObject_RichCompareBool(PyMethod_GET_FUNCTION(self),
                                         PyMethod_GET_FUNCTION(other), op);
        }
        if (b == -1) {
            return NULL;
        }
    }
    PyObject *res = b ? Py_True : Py_False;
    Py_INCREF(res);
    return res;
}


static PyObject *
method_repr(PyMethodObject *a)
{
    PyObject *func = a->im_func;
    PyObject *funcname, *result;
    const char *defname = "?";

    if (_PyObject_LookupAttrId(func, &PyId___qualname__, &funcname) < 0 ||
        (funcname == NULL &&
         _PyObject_LookupAttrId(func, &PyId___name__, &funcname) < 0))
    {
        return NULL;
    }

    if (funcname != NULL && !PyUnicode_Check(funcname)) {
        Py_DECREF(funcname);
        funcname = NULL;
    }

    /* XXX Shouldn't use repr()/%R here! */
    result = PyUnicode_FromFormat("<bound method %V of %R>",
                                  funcname, defname,
                                  PyMethod_GET_SELF(a));

    Py_XDECREF(funcname);
    return result;
}


static Py_hash_t
method_hash(PyMethodObject *a)
{
    Py_uhash_t mult = _PyHASH_MULTIPLIER, err = -1;

    Py_uhash_t h = PyBaseFunction_Type.tp_hash((PyObject *)a);
    if (h == err) {
        return -1;
    }

    Py_uhash_t t = PyObject_Hash(a->im_func);
    if (t == err) {
        return -1;
    }
    h = (h * mult) + t;

    if (h == err) {
        h--;
    }
    return h;
}


static int
method_traverse(PyMethodObject *im, visitproc visit, void *arg)
{
    PyBaseFunction_Type.tp_traverse((PyObject*)im, visit, arg);
    Py_VISIT(im->im_func);
    return 0;
}


PyTypeObject PyMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "bound_method",
    sizeof(PyMethodObject),
    0,
    (destructor)method_dealloc,                 /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)method_repr,                      /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    (hashfunc)method_hash,                      /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    method_getattro,                            /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASEFUNCTION,                /* tp_flags */
    method_doc,                                 /* tp_doc */
    (traverseproc)method_traverse,              /* tp_traverse */
    _PyBaseFunction_Clear,                      /* tp_clear */
    (richcmpfunc)method_richcompare,            /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    method_methods,                             /* tp_methods */
    method_memberlist,                          /* tp_members */
    method_getset,                              /* tp_getset */
    &PyBaseFunction_Type,                       /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    method_new,                                 /* tp_new */
};

/* Clear out the free list */

int
PyMethod_ClearFreeList(void)
{
    int freelist_size = numfree;

    while (free_list) {
        PyMethodObject *im = free_list;
        free_list = (PyMethodObject *)(im->next_free);
        PyObject_GC_Del(im);
        numfree--;
    }
    assert(numfree == 0);
    return freelist_size;
}

void
PyMethod_Fini(void)
{
    (void)PyMethod_ClearFreeList();
}

/* Print summary info about the state of the optimized allocator */
void
_PyMethod_DebugMallocStats(FILE *out)
{
    _PyDebugAllocatorStats(out,
                           "free PyMethodObject",
                           numfree, sizeof(PyMethodObject));
}

/* ------------------------------------------------------------------------
 * instance method
 */

PyObject *
PyInstanceMethod_New(PyObject *func) {
    PyInstanceMethodObject *method;
    method = PyObject_GC_New(PyInstanceMethodObject,
                             &PyInstanceMethod_Type);
    if (method == NULL) return NULL;
    Py_INCREF(func);
    method->func = func;
    _PyObject_GC_TRACK(method);
    return (PyObject *)method;
}

PyObject *
PyInstanceMethod_Function(PyObject *im)
{
    if (!PyInstanceMethod_Check(im)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return PyInstanceMethod_GET_FUNCTION(im);
}

#define IMO_OFF(x) offsetof(PyInstanceMethodObject, x)

static PyMemberDef instancemethod_memberlist[] = {
    {"__func__", T_OBJECT, IMO_OFF(func), READONLY|RESTRICTED,
     "the function (or other callable) implementing a method"},
    {NULL}      /* Sentinel */
};

static PyObject *
instancemethod_get_doc(PyObject *self, void *context)
{
    static PyObject *docstr;
    if (docstr == NULL) {
        docstr = PyUnicode_InternFromString("__doc__");
        if (docstr == NULL)
            return NULL;
    }
    return PyObject_GetAttr(PyInstanceMethod_GET_FUNCTION(self), docstr);
}

static PyGetSetDef instancemethod_getset[] = {
    {"__doc__", (getter)instancemethod_get_doc, NULL, NULL},
    {0}
};

static PyObject *
instancemethod_getattro(PyObject *self, PyObject *name)
{
    PyTypeObject *tp = self->ob_type;
    PyObject *descr = NULL;

    if (tp->tp_dict == NULL) {
        if (PyType_Ready(tp) < 0)
            return NULL;
    }
    descr = _PyType_Lookup(tp, name);

    if (descr != NULL) {
        descrgetfunc f = TP_DESCR_GET(descr->ob_type);
        if (f != NULL)
            return f(descr, self, (PyObject *)self->ob_type);
        else {
            Py_INCREF(descr);
            return descr;
        }
    }

    return PyObject_GetAttr(PyInstanceMethod_GET_FUNCTION(self), name);
}

static void
instancemethod_dealloc(PyObject *self) {
    _PyObject_GC_UNTRACK(self);
    Py_DECREF(PyInstanceMethod_GET_FUNCTION(self));
    PyObject_GC_Del(self);
}

static int
instancemethod_traverse(PyObject *self, visitproc visit, void *arg) {
    Py_VISIT(PyInstanceMethod_GET_FUNCTION(self));
    return 0;
}

static PyObject *
instancemethod_call(PyObject *self, PyObject *arg, PyObject *kw)
{
    return PyObject_Call(PyMethod_GET_FUNCTION(self), arg, kw);
}

static PyObject *
instancemethod_descr_get(PyObject *descr, PyObject *obj, PyObject *type) {
    PyObject *func = PyInstanceMethod_GET_FUNCTION(descr);
    if (obj == NULL) {
        Py_INCREF(func);
        return func;
    }
    else
        return PyMethod_New(func, obj);
}

static PyObject *
instancemethod_richcompare(PyObject *self, PyObject *other, int op)
{
    PyInstanceMethodObject *a, *b;
    PyObject *res;
    int eq;

    if ((op != Py_EQ && op != Py_NE) ||
        !PyInstanceMethod_Check(self) ||
        !PyInstanceMethod_Check(other))
    {
        Py_RETURN_NOTIMPLEMENTED;
    }
    a = (PyInstanceMethodObject *)self;
    b = (PyInstanceMethodObject *)other;
    eq = PyObject_RichCompareBool(a->func, b->func, Py_EQ);
    if (eq < 0)
        return NULL;
    if (op == Py_EQ)
        res = eq ? Py_True : Py_False;
    else
        res = eq ? Py_False : Py_True;
    Py_INCREF(res);
    return res;
}

static PyObject *
instancemethod_repr(PyObject *self)
{
    PyObject *func = PyInstanceMethod_Function(self);
    PyObject *funcname, *result;
    const char *defname = "?";

    if (func == NULL) {
        PyErr_BadInternalCall();
        return NULL;
    }

    if (_PyObject_LookupAttrId(func, &PyId___name__, &funcname) < 0) {
        return NULL;
    }
    if (funcname != NULL && !PyUnicode_Check(funcname)) {
        Py_DECREF(funcname);
        funcname = NULL;
    }

    result = PyUnicode_FromFormat("<instancemethod %V at %p>",
                                  funcname, defname, self);

    Py_XDECREF(funcname);
    return result;
}

/*
static long
instancemethod_hash(PyObject *self)
{
    long x, y;
    x = (long)self;
    y = PyObject_Hash(PyInstanceMethod_GET_FUNCTION(self));
    if (y == -1)
        return -1;
    x = x ^ y;
    if (x == -1)
        x = -2;
    return x;
}
*/

PyDoc_STRVAR(instancemethod_doc,
"instancemethod(function)\n\
\n\
Bind a function to a class.");

static PyObject *
instancemethod_new(PyTypeObject* type, PyObject* args, PyObject *kw)
{
    PyObject *func;

    if (!_PyArg_NoKeywords("instancemethod", kw))
        return NULL;
    if (!PyArg_UnpackTuple(args, "instancemethod", 1, 1, &func))
        return NULL;
    if (!PyCallable_Check(func)) {
        PyErr_SetString(PyExc_TypeError,
                        "first argument must be callable");
        return NULL;
    }

    return PyInstanceMethod_New(func);
}

PyTypeObject PyInstanceMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "instancemethod",                           /* tp_name */
    sizeof(PyInstanceMethodObject),             /* tp_basicsize */
    0,                                          /* tp_itemsize */
    instancemethod_dealloc,                     /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)instancemethod_repr,              /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0, /*(hashfunc)instancemethod_hash,         tp_hash  */
    instancemethod_call,                        /* tp_call */
    0,                                          /* tp_str */
    instancemethod_getattro,                    /* tp_getattro */
    PyObject_GenericSetAttr,                    /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT
        | Py_TPFLAGS_HAVE_GC,                   /* tp_flags */
    instancemethod_doc,                         /* tp_doc */
    instancemethod_traverse,                    /* tp_traverse */
    0,                                          /* tp_clear */
    instancemethod_richcompare,                 /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    instancemethod_memberlist,                  /* tp_members */
    instancemethod_getset,                      /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    instancemethod_descr_get,                   /* tp_descr_get */
    0,                                          /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    instancemethod_new,                         /* tp_new */
};
