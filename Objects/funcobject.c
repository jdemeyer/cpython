
/* Function object implementation */

#include "Python.h"
#include "internal/mem.h"
#include "internal/pystate.h"
#include "code.h"
#include "structmember.h"


static void
dfunc_dealloc(PyDefinedFunctionObject *op)
{
    _PyObject_GC_UNTRACK(op);
    if (op->base.m_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)op);
    }
    _PyBaseFunction_Clear((PyObject *)op);
    Py_XDECREF(op->func_dict);
    PyObject_GC_Del(op);
}


static int
dfunc_traverse(PyDefinedFunctionObject *f, visitproc visit, void *arg)
{
    PyBaseFunction_Type.tp_traverse((PyObject *)f, visit, arg);
    Py_VISIT(f->func_dict);
    return 0;
}


PyObject *
PyFunction_NewPython(PyTypeObject *cls, PyObject *code,
                     PyObject *globals, PyObject *name, PyObject *qualname)
{
    PyFunctionObject *op;
    PyObject *doc, *consts, *module;
    static PyObject *__name__ = NULL;

    if (__name__ == NULL) {
        __name__ = PyUnicode_InternFromString("__name__");
        if (__name__ == NULL)
            return NULL;
    }

    if (name == NULL) {
        name = ((PyCodeObject *)code)->co_name;
    }
    if (qualname == NULL) {
        qualname = name;
    }
    const char *utf8name = PyUnicode_AsUTF8(name);
    if (utf8name == NULL) {
        return NULL;
    }

    /* __module__: If module name is in globals, use it. */
    module = PyDict_GetItem(globals, __name__);

    op = (PyFunctionObject *)PyBaseFunction_New(cls, NULL, NULL, module, NULL);
    if (op == NULL) {
        return NULL;
    }

    op->base.m_ml = &op->_ml;
    op->base.m_ml->ml_name = utf8name;
    op->base.m_ml->ml_flags = METH_FASTCALL | METH_KEYWORDS |
                              METH_PASS_FUNCTION | METH_CALL_UNBOUND |
                              METH_PYTHON;
    op->base.m_ml->ml_meth = (PyCFunction)_PyFunction_FastCallSelf;

    Py_INCREF(name);
    op->func_name = name;
    Py_INCREF(qualname);
    op->func_qualname = qualname;
    Py_INCREF(code);
    op->func_code = code;
    Py_INCREF(globals);
    op->func_globals = globals;

    consts = ((PyCodeObject *)code)->co_consts;
    if (PyTuple_Size(consts) >= 1) {
        doc = PyTuple_GetItem(consts, 0);
        if (!PyUnicode_Check(doc))
            doc = Py_None;
    }
    else
        doc = Py_None;
    Py_INCREF(doc);
    op->func_doc = doc;

    /* func_defaults, func_kwdefaults, func_closure, func_annotations, func_dict
       remain NULL */

    return (PyObject *)op;
}


PyObject *
PyFunction_Copy(PyTypeObject *cls, PyObject *op)
{
    PyFunctionObject* src = (PyFunctionObject *)op;

    PyFunctionObject *res;
    res = (PyFunctionObject *)PyBaseFunction_New(cls, NULL,
                                                 src->base.m_self,
                                                 src->base.m_module,
                                                 src->base.m_parent);
    if (res == NULL) {
        return NULL;
    }

    res->base.m_ml = &res->_ml;
    res->base.m_ml->ml_name = src->base.m_ml->ml_name;
    res->base.m_ml->ml_flags = src->base.m_ml->ml_flags;
    res->base.m_ml->ml_meth = src->base.m_ml->ml_meth;

    Py_INCREF(src->func_name);
    res->func_name = src->func_name;
    Py_INCREF(src->func_qualname);
    res->func_qualname = src->func_qualname;
    Py_INCREF(src->func_code);
    res->func_code = src->func_code;
    Py_INCREF(src->func_globals);
    res->func_globals = src->func_globals;
    Py_XINCREF(src->func_doc);
    res->func_doc = src->func_doc;
    Py_XINCREF(src->func_defaults);
    res->func_defaults = src->func_defaults;
    Py_XINCREF(src->func_closure);
    res->func_closure = src->func_closure;

    res->func_kwdefaults = src->func_kwdefaults ?
                           PyDict_Copy(src->func_kwdefaults) : NULL;
    res->func_annotations = src->func_annotations ?
                            PyDict_Copy(src->func_annotations) : NULL;
    res->func_dict = src->func_dict ?
                     PyDict_Copy(src->func_dict) : NULL;

    return (PyObject *)res;
}


PyObject *
PyFunction_GetCode(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_code;
}

PyObject *
PyFunction_GetGlobals(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_globals;
}

PyObject *
PyFunction_GetModule(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyBaseFunctionObject *)op)->m_module;
}

PyObject *
PyFunction_GetDefaults(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_defaults;
}

int
PyFunction_SetDefaults(PyObject *op, PyObject *defaults)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (defaults == Py_None)
        defaults = NULL;
    else if (defaults && PyTuple_Check(defaults)) {
        Py_INCREF(defaults);
    }
    else {
        PyErr_SetString(PyExc_SystemError, "non-tuple default args");
        return -1;
    }
    Py_XSETREF(((PyFunctionObject *)op)->func_defaults, defaults);
    return 0;
}

PyObject *
PyFunction_GetKwDefaults(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_kwdefaults;
}

int
PyFunction_SetKwDefaults(PyObject *op, PyObject *defaults)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (defaults == Py_None)
        defaults = NULL;
    else if (defaults && PyDict_Check(defaults)) {
        Py_INCREF(defaults);
    }
    else {
        PyErr_SetString(PyExc_SystemError,
                        "non-dict keyword only default args");
        return -1;
    }
    Py_XSETREF(((PyFunctionObject *)op)->func_kwdefaults, defaults);
    return 0;
}

PyObject *
PyFunction_GetClosure(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_closure;
}

int
PyFunction_SetClosure(PyObject *op, PyObject *closure)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (closure == Py_None)
        closure = NULL;
    else if (PyTuple_Check(closure)) {
        Py_INCREF(closure);
    }
    else {
        PyErr_Format(PyExc_SystemError,
                     "expected tuple for closure, got '%.100s'",
                     closure->ob_type->tp_name);
        return -1;
    }
    Py_XSETREF(((PyFunctionObject *)op)->func_closure, closure);
    return 0;
}

PyObject *
PyFunction_GetAnnotations(PyObject *op)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return NULL;
    }
    return ((PyFunctionObject *) op) -> func_annotations;
}

int
PyFunction_SetAnnotations(PyObject *op, PyObject *annotations)
{
    if (!PyFunction_Check(op)) {
        PyErr_BadInternalCall();
        return -1;
    }
    if (annotations == Py_None)
        annotations = NULL;
    else if (annotations && PyDict_Check(annotations)) {
        Py_INCREF(annotations);
    }
    else {
        PyErr_SetString(PyExc_SystemError,
                        "non-dict annotations");
        return -1;
    }
    Py_XSETREF(((PyFunctionObject *)op)->func_annotations, annotations);
    return 0;
}

/* Methods */

#define OFF(x) offsetof(PyFunctionObject, x)

static PyMemberDef func_memberlist[] = {
    {"__closure__",   T_OBJECT,     OFF(func_closure), RESTRICTED|READONLY},
    {"__globals__",   T_OBJECT,     OFF(func_globals), RESTRICTED|READONLY},
    {"__doc__",       T_OBJECT,     OFF(func_doc), PY_WRITE_RESTRICTED},
    {NULL}  /* Sentinel */
};

static PyObject *
func_get_code(PyFunctionObject *op)
{
    Py_INCREF(op->func_code);
    return op->func_code;
}

static int
func_set_code(PyFunctionObject *op, PyObject *value)
{
    Py_ssize_t nfree, nclosure;

    /* Not legal to del f.func_code or to set it to anything
     * other than a code object. */
    if (value == NULL || !PyCode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__code__ must be set to a code object");
        return -1;
    }
    nfree = PyCode_GetNumFree((PyCodeObject *)value);
    nclosure = (op->func_closure == NULL ? 0 :
            PyTuple_GET_SIZE(op->func_closure));
    if (nclosure != nfree) {
        PyErr_Format(PyExc_ValueError,
                     "%U() requires a code object with %zd free vars,"
                     " not %zd",
                     op->func_name,
                     nclosure, nfree);
        return -1;
    }
    Py_INCREF(value);
    Py_XSETREF(op->func_code, value);
    return 0;
}

static PyObject *
func_get_name(PyFunctionObject *op)
{
    Py_INCREF(op->func_name);
    return op->func_name;
}

static int
func_set_name(PyFunctionObject *op, PyObject *value)
{
    /* Not legal to del f.func_name or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__name__ must be set to a string object");
        return -1;
    }
    const char *utf8name = PyUnicode_AsUTF8(value);
    if (utf8name == NULL) {
        return -1;
    }
    Py_INCREF(value);
    Py_XSETREF(op->func_name, value);
    op->base.m_ml->ml_name = utf8name;
    return 0;
}

static PyObject *
func_get_qualname(PyFunctionObject *op)
{
    Py_INCREF(op->func_qualname);
    return op->func_qualname;
}

static int
func_set_qualname(PyFunctionObject *op, PyObject *value)
{
    /* Not legal to del f.__qualname__ or to set it to anything
     * other than a string object. */
    if (value == NULL || !PyUnicode_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__qualname__ must be set to a string object");
        return -1;
    }
    Py_INCREF(value);
    Py_XSETREF(op->func_qualname, value);
    return 0;
}

static PyObject *
func_get_defaults(PyFunctionObject *op)
{
    if (op->func_defaults == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(op->func_defaults);
    return op->func_defaults;
}

static int
func_set_defaults(PyFunctionObject *op, PyObject *value)
{
    /* Legal to del f.func_defaults.
     * Can only set func_defaults to NULL or a tuple. */
    if (value == Py_None)
        value = NULL;
    if (value != NULL && !PyTuple_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
                        "__defaults__ must be set to a tuple object");
        return -1;
    }
    Py_XINCREF(value);
    Py_XSETREF(op->func_defaults, value);
    return 0;
}

static PyObject *
func_get_kwdefaults(PyFunctionObject *op)
{
    if (op->func_kwdefaults == NULL) {
        Py_RETURN_NONE;
    }
    Py_INCREF(op->func_kwdefaults);
    return op->func_kwdefaults;
}

static int
func_set_kwdefaults(PyFunctionObject *op, PyObject *value)
{
    if (value == Py_None)
        value = NULL;
    /* Legal to del f.func_kwdefaults.
     * Can only set func_kwdefaults to NULL or a dict. */
    if (value != NULL && !PyDict_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
            "__kwdefaults__ must be set to a dict object");
        return -1;
    }
    Py_XINCREF(value);
    Py_XSETREF(op->func_kwdefaults, value);
    return 0;
}

static PyObject *
func_get_annotations(PyFunctionObject *op)
{
    if (op->func_annotations == NULL) {
        op->func_annotations = PyDict_New();
        if (op->func_annotations == NULL)
            return NULL;
    }
    Py_INCREF(op->func_annotations);
    return op->func_annotations;
}

static int
func_set_annotations(PyFunctionObject *op, PyObject *value)
{
    if (value == Py_None)
        value = NULL;
    /* Legal to del f.func_annotations.
     * Can only set func_annotations to NULL (through C api)
     * or a dict. */
    if (value != NULL && !PyDict_Check(value)) {
        PyErr_SetString(PyExc_TypeError,
            "__annotations__ must be set to a dict object");
        return -1;
    }
    Py_XINCREF(value);
    Py_XSETREF(op->func_annotations, value);
    return 0;
}

static PyGetSetDef func_getsetlist[] = {
    {"__code__", (getter)func_get_code, (setter)func_set_code},
    {"__defaults__", (getter)func_get_defaults,
     (setter)func_set_defaults},
    {"__kwdefaults__", (getter)func_get_kwdefaults,
     (setter)func_set_kwdefaults},
    {"__annotations__", (getter)func_get_annotations,
     (setter)func_set_annotations},
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict},
    {"__name__", (getter)func_get_name, (setter)func_set_name},
    {"__qualname__", (getter)func_get_qualname, (setter)func_set_qualname},
    {NULL} /* Sentinel */
};

/*[clinic input]
class function "PyFunctionObject *" "&PyFunction_Type"
[clinic start generated code]*/
/*[clinic end generated code: output=da39a3ee5e6b4b0d input=70af9c90aa2e71b0]*/

#include "clinic/funcobject.c.h"

/* function.__new__() maintains the following invariants for closures.
   The closure must correspond to the free variables of the code object.

   if len(code.co_freevars) == 0:
       closure = NULL
   else:
       len(closure) == len(code.co_freevars)
   for every elt in closure, type(elt) == cell
*/

/*[clinic input]
@classmethod
function.__new__ as func_new
    code: object(type="PyCodeObject *", subclass_of="&PyCode_Type")
        a code object
    globals: object(subclass_of="&PyDict_Type")
        the globals dictionary
    name: object = None
        a string that overrides the name from the code object
    argdefs as defaults: object = None
        a tuple that specifies the default argument values
    closure: object = None
        a tuple that supplies the bindings for free variables

Create a function object.
[clinic start generated code]*/

static PyObject *
func_new_impl(PyTypeObject *type, PyCodeObject *code, PyObject *globals,
              PyObject *name, PyObject *defaults, PyObject *closure)
/*[clinic end generated code: output=99c6d9da3a24e3be input=93611752fc2daf11]*/
{
    PyFunctionObject *newfunc;
    Py_ssize_t nfree, nclosure;

    if (name == Py_None) {
        name = NULL;
    }
    else {
        if (!PyUnicode_Check(name)) {
            PyErr_SetString(PyExc_TypeError,
                            "arg 3 (name) must be None or string");
            return NULL;
        }
    }
    if (defaults != Py_None && !PyTuple_Check(defaults)) {
        PyErr_SetString(PyExc_TypeError,
                        "arg 4 (defaults) must be None or tuple");
        return NULL;
    }
    nfree = PyTuple_GET_SIZE(code->co_freevars);
    if (!PyTuple_Check(closure)) {
        if (nfree && closure == Py_None) {
            PyErr_SetString(PyExc_TypeError,
                            "arg 5 (closure) must be tuple");
            return NULL;
        }
        else if (closure != Py_None) {
            PyErr_SetString(PyExc_TypeError,
                "arg 5 (closure) must be None or tuple");
            return NULL;
        }
    }

    /* check that the closure is well-formed */
    nclosure = closure == Py_None ? 0 : PyTuple_GET_SIZE(closure);
    if (nfree != nclosure)
        return PyErr_Format(PyExc_ValueError,
                            "%U requires closure of length %zd, not %zd",
                            code->co_name, nfree, nclosure);
    if (nclosure) {
        Py_ssize_t i;
        for (i = 0; i < nclosure; i++) {
            PyObject *o = PyTuple_GET_ITEM(closure, i);
            if (!PyCell_Check(o)) {
                return PyErr_Format(PyExc_TypeError,
                    "arg 5 (closure) expected cell, found %s",
                                    o->ob_type->tp_name);
            }
        }
    }

    newfunc = (PyFunctionObject *)PyFunction_NewPython(type,
                                                       (PyObject *)code,
                                                       globals,
                                                       name, name);
    if (newfunc == NULL)
        return NULL;

    if (defaults != Py_None) {
        Py_INCREF(defaults);
        newfunc->func_defaults = defaults;
    }
    if (closure != Py_None) {
        Py_INCREF(closure);
        newfunc->func_closure = closure;
    }

    return (PyObject *)newfunc;
}

static PyObject *
func_new_wrapper(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    /* Check for a single positional argument, namely a function to copy */
    if (PyTuple_GET_SIZE(args) == 1 &&
        (kwds == NULL || PyDict_GET_SIZE(kwds) == 0))
    {
        PyObject *arg1 = PyTuple_GET_ITEM(args, 0);
        if (PyBaseFunction_Check(arg1)) {
            if (!PyBaseFunction_HAS_FLAG(arg1, METH_PYTHON)) {
                PyErr_SetString(PyExc_TypeError,
                                "only Python functions can be copied");
                return NULL;
            }
            return PyFunction_Copy(type, arg1);
        }
    }
    return func_new(type, args, kwds);
}


static void
func_dealloc(PyFunctionObject *op)
{
    _PyObject_GC_UNTRACK(op);
    if (op->base.m_weakreflist != NULL) {
        PyObject_ClearWeakRefs((PyObject *)op);
    }
    _PyBaseFunction_Clear((PyObject *)op);
    Py_XDECREF(op->func_dict);
    Py_XDECREF(op->func_name);
    Py_XDECREF(op->func_qualname);
    Py_XDECREF(op->func_code);
    Py_XDECREF(op->func_globals);
    Py_XDECREF(op->func_doc);
    Py_XDECREF(op->func_defaults);
    Py_XDECREF(op->func_kwdefaults);
    Py_XDECREF(op->func_closure);
    Py_XDECREF(op->func_annotations);
    PyObject_GC_Del(op);
}

static PyObject*
func_repr(PyFunctionObject *op)
{
    return PyUnicode_FromFormat("<%s %U at %p>",
                               Py_TYPE(op)->tp_name,
                               op->func_qualname, op);
}

static int
func_traverse(PyFunctionObject *f, visitproc visit, void *arg)
{
    PyBaseFunction_Type.tp_traverse((PyObject *)f, visit, arg);
    Py_VISIT(f->func_code);
    Py_VISIT(f->func_globals);
    Py_VISIT(f->func_defaults);
    Py_VISIT(f->func_kwdefaults);
    Py_VISIT(f->func_doc);
    Py_VISIT(f->func_name);
    Py_VISIT(f->func_dict);
    Py_VISIT(f->func_closure);
    Py_VISIT(f->func_annotations);
    Py_VISIT(f->func_qualname);
    return 0;
}


static int
func_clear(PyFunctionObject *f)
{
    _PyBaseFunction_Clear((PyObject *)f);
    Py_CLEAR(f->func_code);
    Py_CLEAR(f->func_globals);
    Py_CLEAR(f->func_defaults);
    Py_CLEAR(f->func_kwdefaults);
    Py_CLEAR(f->func_doc);
    Py_CLEAR(f->func_name);
    Py_CLEAR(f->func_dict);
    Py_CLEAR(f->func_closure);
    Py_CLEAR(f->func_annotations);
    Py_CLEAR(f->func_qualname);
    return 0;
}


PyTypeObject PyDefinedFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "defined_function",
    sizeof(PyDefinedFunctionObject),
    0,
    (destructor)dfunc_dealloc,                  /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_BASEFUNCTION,                /* tp_flags */
    0,                                          /* tp_doc */
    (traverseproc)dfunc_traverse,               /* tp_traverse */
    _PyBaseFunction_Clear,                      /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    0,                                          /* tp_members */
    0,                                          /* tp_getset */
    &PyBaseFunction_Type,                       /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    OFF(func_dict),                             /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    0,                                          /* tp_new */
};


PyTypeObject PyFunction_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "function",
    sizeof(PyFunctionObject),
    0,
    (destructor)func_dealloc,                   /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    (reprfunc)func_repr,                        /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_GC |
        Py_TPFLAGS_BASETYPE |
        Py_TPFLAGS_BASEFUNCTION,                /* tp_flags */
    func_new__doc__,                            /* tp_doc */
    (traverseproc)func_traverse,                /* tp_traverse */
    (inquiry)func_clear,                        /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    func_memberlist,                            /* tp_members */
    func_getsetlist,                            /* tp_getset */
    &PyDefinedFunction_Type,                    /* tp_base */
    0,                                          /* tp_dict */
    0,                                          /* tp_descr_get */
    0,                                          /* tp_descr_set */
    OFF(func_dict),                             /* tp_dictoffset */
    0,                                          /* tp_init */
    0,                                          /* tp_alloc */
    func_new_wrapper,                           /* tp_new */
};


/* Class method object */

/* A class method receives the class as implicit first argument,
   just like an instance method receives the instance.
   To declare a class method, use this idiom:

     class C:
         @classmethod
         def f(cls, arg1, arg2, ...):
             ...

   It can be called either on the class (e.g. C.f()) or on an instance
   (e.g. C().f()); the instance is ignored except for its class.
   If a class method is called for a derived class, the derived class
   object is passed as the implied first argument.

   Class methods are different than C++ or Java static methods.
   If you want those, see static methods below.
*/

typedef struct {
    PyObject_HEAD
    PyObject *cm_callable;
    PyObject *cm_dict;
} classmethod;

static void
cm_dealloc(classmethod *cm)
{
    _PyObject_GC_UNTRACK((PyObject *)cm);
    Py_XDECREF(cm->cm_callable);
    Py_XDECREF(cm->cm_dict);
    Py_TYPE(cm)->tp_free((PyObject *)cm);
}

static int
cm_traverse(classmethod *cm, visitproc visit, void *arg)
{
    Py_VISIT(cm->cm_callable);
    Py_VISIT(cm->cm_dict);
    return 0;
}

static int
cm_clear(classmethod *cm)
{
    Py_CLEAR(cm->cm_callable);
    Py_CLEAR(cm->cm_dict);
    return 0;
}


static PyObject *
cm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    classmethod *cm = (classmethod *)self;

    if (cm->cm_callable == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "uninitialized classmethod object");
        return NULL;
    }
    if (type == NULL)
        type = (PyObject *)(Py_TYPE(obj));
    return PyMethod_New(cm->cm_callable, type);
}

static int
cm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    classmethod *cm = (classmethod *)self;
    PyObject *callable;

    if (!_PyArg_NoKeywords("classmethod", kwds))
        return -1;
    if (!PyArg_UnpackTuple(args, "classmethod", 1, 1, &callable))
        return -1;
    Py_INCREF(callable);
    Py_XSETREF(cm->cm_callable, callable);
    return 0;
}

static PyMemberDef cm_memberlist[] = {
    {"__func__", T_OBJECT, offsetof(classmethod, cm_callable), READONLY},
    {NULL}  /* Sentinel */
};

static PyObject *
cm_get___isabstractmethod__(classmethod *cm, void *closure)
{
    int res = _PyObject_IsAbstract(cm->cm_callable);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyGetSetDef cm_getsetlist[] = {
    {"__isabstractmethod__",
     (getter)cm_get___isabstractmethod__, NULL,
     NULL,
     NULL},
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict, NULL, NULL},
    {NULL} /* Sentinel */
};

PyDoc_STRVAR(classmethod_doc,
"classmethod(function) -> method\n\
\n\
Convert a function to be a class method.\n\
\n\
A class method receives the class as implicit first argument,\n\
just like an instance method receives the instance.\n\
To declare a class method, use this idiom:\n\
\n\
  class C:\n\
      @classmethod\n\
      def f(cls, arg1, arg2, ...):\n\
          ...\n\
\n\
It can be called either on the class (e.g. C.f()) or on an instance\n\
(e.g. C().f()).  The instance is ignored except for its class.\n\
If a class method is called for a derived class, the derived class\n\
object is passed as the implied first argument.\n\
\n\
Class methods are different than C++ or Java static methods.\n\
If you want those, see the staticmethod builtin.");

PyTypeObject PyClassMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "classmethod",
    sizeof(classmethod),
    0,
    (destructor)cm_dealloc,                     /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    classmethod_doc,                            /* tp_doc */
    (traverseproc)cm_traverse,                  /* tp_traverse */
    (inquiry)cm_clear,                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    cm_memberlist,              /* tp_members */
    cm_getsetlist,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    cm_descr_get,                               /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(classmethod, cm_dict),             /* tp_dictoffset */
    cm_init,                                    /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

PyObject *
PyClassMethod_New(PyObject *callable)
{
    classmethod *cm = (classmethod *)
        PyType_GenericAlloc(&PyClassMethod_Type, 0);
    if (cm != NULL) {
        Py_INCREF(callable);
        cm->cm_callable = callable;
    }
    return (PyObject *)cm;
}


/* Static method object */

/* A static method does not receive an implicit first argument.
   To declare a static method, use this idiom:

     class C:
         @staticmethod
         def f(arg1, arg2, ...):
             ...

   It can be called either on the class (e.g. C.f()) or on an instance
   (e.g. C().f()); the instance is ignored except for its class.

   Static methods in Python are similar to those found in Java or C++.
   For a more advanced concept, see class methods above.
*/

typedef struct {
    PyObject_HEAD
    PyObject *sm_callable;
    PyObject *sm_dict;
} staticmethod;

static void
sm_dealloc(staticmethod *sm)
{
    _PyObject_GC_UNTRACK((PyObject *)sm);
    Py_XDECREF(sm->sm_callable);
    Py_XDECREF(sm->sm_dict);
    Py_TYPE(sm)->tp_free((PyObject *)sm);
}

static int
sm_traverse(staticmethod *sm, visitproc visit, void *arg)
{
    Py_VISIT(sm->sm_callable);
    Py_VISIT(sm->sm_dict);
    return 0;
}

static int
sm_clear(staticmethod *sm)
{
    Py_CLEAR(sm->sm_callable);
    Py_CLEAR(sm->sm_dict);
    return 0;
}

static PyObject *
sm_descr_get(PyObject *self, PyObject *obj, PyObject *type)
{
    staticmethod *sm = (staticmethod *)self;

    if (sm->sm_callable == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "uninitialized staticmethod object");
        return NULL;
    }
    Py_INCREF(sm->sm_callable);
    return sm->sm_callable;
}

static int
sm_init(PyObject *self, PyObject *args, PyObject *kwds)
{
    staticmethod *sm = (staticmethod *)self;
    PyObject *callable;

    if (!_PyArg_NoKeywords("staticmethod", kwds))
        return -1;
    if (!PyArg_UnpackTuple(args, "staticmethod", 1, 1, &callable))
        return -1;
    Py_INCREF(callable);
    Py_XSETREF(sm->sm_callable, callable);
    return 0;
}

static PyMemberDef sm_memberlist[] = {
    {"__func__", T_OBJECT, offsetof(staticmethod, sm_callable), READONLY},
    {NULL}  /* Sentinel */
};

static PyObject *
sm_get___isabstractmethod__(staticmethod *sm, void *closure)
{
    int res = _PyObject_IsAbstract(sm->sm_callable);
    if (res == -1) {
        return NULL;
    }
    else if (res) {
        Py_RETURN_TRUE;
    }
    Py_RETURN_FALSE;
}

static PyGetSetDef sm_getsetlist[] = {
    {"__isabstractmethod__",
     (getter)sm_get___isabstractmethod__, NULL,
     NULL,
     NULL},
    {"__dict__", PyObject_GenericGetDict, PyObject_GenericSetDict, NULL, NULL},
    {NULL} /* Sentinel */
};

PyDoc_STRVAR(staticmethod_doc,
"staticmethod(function) -> method\n\
\n\
Convert a function to be a static method.\n\
\n\
A static method does not receive an implicit first argument.\n\
To declare a static method, use this idiom:\n\
\n\
     class C:\n\
         @staticmethod\n\
         def f(arg1, arg2, ...):\n\
             ...\n\
\n\
It can be called either on the class (e.g. C.f()) or on an instance\n\
(e.g. C().f()).  The instance is ignored except for its class.\n\
\n\
Static methods in Python are similar to those found in Java or C++.\n\
For a more advanced concept, see the classmethod builtin.");

PyTypeObject PyStaticMethod_Type = {
    PyVarObject_HEAD_INIT(&PyType_Type, 0)
    "staticmethod",
    sizeof(staticmethod),
    0,
    (destructor)sm_dealloc,                     /* tp_dealloc */
    0,                                          /* tp_print */
    0,                                          /* tp_getattr */
    0,                                          /* tp_setattr */
    0,                                          /* tp_reserved */
    0,                                          /* tp_repr */
    0,                                          /* tp_as_number */
    0,                                          /* tp_as_sequence */
    0,                                          /* tp_as_mapping */
    0,                                          /* tp_hash */
    0,                                          /* tp_call */
    0,                                          /* tp_str */
    0,                                          /* tp_getattro */
    0,                                          /* tp_setattro */
    0,                                          /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE | Py_TPFLAGS_HAVE_GC,
    staticmethod_doc,                           /* tp_doc */
    (traverseproc)sm_traverse,                  /* tp_traverse */
    (inquiry)sm_clear,                          /* tp_clear */
    0,                                          /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    0,                                          /* tp_iter */
    0,                                          /* tp_iternext */
    0,                                          /* tp_methods */
    sm_memberlist,              /* tp_members */
    sm_getsetlist,                              /* tp_getset */
    0,                                          /* tp_base */
    0,                                          /* tp_dict */
    sm_descr_get,                               /* tp_descr_get */
    0,                                          /* tp_descr_set */
    offsetof(staticmethod, sm_dict),            /* tp_dictoffset */
    sm_init,                                    /* tp_init */
    PyType_GenericAlloc,                        /* tp_alloc */
    PyType_GenericNew,                          /* tp_new */
    PyObject_GC_Del,                            /* tp_free */
};

PyObject *
PyStaticMethod_New(PyObject *callable)
{
    staticmethod *sm = (staticmethod *)
        PyType_GenericAlloc(&PyStaticMethod_Type, 0);
    if (sm != NULL) {
        Py_INCREF(callable);
        sm->sm_callable = callable;
    }
    return (PyObject *)sm;
}
