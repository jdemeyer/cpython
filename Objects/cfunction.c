/* Built-in function implementation */

#include "Python.h"
#include "internal/mem.h"
#include "internal/pystate.h"
#include "structmember.h"


PyObject *
PyCFunction_NewEx(PyMethodDef *ml, PyObject *self, PyObject *module)
{
    PyObject *parent = NULL;

    if (self == NULL) {
        self = Py_None;
    }
    else if (PyModule_Check(self)) {
        parent = self;
    }

    return PyBaseFunction_New(&PyCFunction_Type, (PyCFunctionDef*)ml,
                              self, module, parent);
}


/* Methods */

/* For backwards compatibility, override base_function.__self__ to
   return None instead of raising AttributeError. */
static PyObject *
meth_get__self__(PyObject *m, void *closure)
{
    PyObject *self = PyBaseFunction_GET_SELF(m);

    if (self == NULL) {
        self = Py_None;
    }
    Py_INCREF(self);
    return self;
}

static PyObject *
meth_get__text_signature__(PyCFunctionObject *m, void *closure)
{
    return _PyType_GetTextSignatureFromInternalDoc(m->m_ml->ml_name, m->m_ml->ml_doc);
}

static PyObject *
meth_get__doc__(PyCFunctionObject *m, void *closure)
{
    return _PyType_GetDocFromInternalDoc(m->m_ml->ml_name, m->m_ml->ml_doc);
}

static PyGetSetDef meth_getsets [] = {
    {"__doc__",  (getter)meth_get__doc__,  NULL, NULL},
    {"__text_signature__", (getter)meth_get__text_signature__, NULL, NULL},
    {"__self__", (getter)meth_get__self__, NULL, NULL},
    {0}
};

static PyObject *
meth_repr(PyCFunctionObject *m)
{
    PyTypeObject *objclass = PyBaseFunction_GET_OBJCLASS(m);
    if (objclass == NULL) {
        return PyUnicode_FromFormat("<built-in function %s>",
                                    m->m_ml->ml_name);
    }
    return PyUnicode_FromFormat("<method '%s' of '%s' objects>",
                                m->m_ml->ml_name, objclass->tp_name);
}


PyTypeObject PyCFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "cfunction",
    sizeof(PyCFunctionObject),
    0,
    0,                                          /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)meth_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    PyObject_GenericGetAttr,                    /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASEFUNCTION,                /* tp_flags */
    0,                                          /* tp_doc */
    0,                                          /* tp_traverse */
    0,                                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    meth_getsets,                               /* tp_getset */
    &PyBaseFunction_Type,                       /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
};


/* Preserve ABI (PEP 384, PEP 575) */

#undef PyCFunction_New
PyAPI_FUNC(PyObject *)
PyCFunction_New(PyMethodDef *ml, PyObject *self)
{
    return PyCFunction_NewEx(ml, self, NULL);
}

#undef PyCFunction_GetFunction
PyAPI_FUNC(PyCFunction)
PyCFunction_GetFunction(PyObject *op)
{
    return PyBaseFunction_GetFunction(op);
}

#undef PyCFunction_GetSelf
PyAPI_FUNC(PyObject *)
PyCFunction_GetSelf(PyObject *op)
{
    return PyBaseFunction_GetSelf(op);
}

#undef PyCFunction_GetFlags
PyAPI_FUNC(int)
PyCFunction_GetFlags(PyObject *op)
{
    return PyBaseFunction_GetFlags(op);
}

#undef PyCFunction_Call
PyAPI_FUNC(PyObject *)
PyCFunction_Call(PyObject *func, PyObject *args, PyObject *kwargs)
{
    return PyBaseFunction_Call(func, args, kwargs);
}
