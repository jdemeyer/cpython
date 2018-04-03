/* base_function object interface */

#ifndef Py_BASEFUNCTION_H
#define Py_BASEFUNCTION_H
#ifdef __cplusplus
extern "C" {
#endif


/* Types for PyCFunctionDef.ml_meth */
typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*PyCFunctionFast)(PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*PyCFunctionFastKeywords)(PyObject *,
                                             PyObject *const *, Py_ssize_t,
                                             PyObject *);

typedef PyObject *(*PyCFunc)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFuncFast)(PyObject *,
                                 PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFuncWithKeywords)(PyObject *,
                                         PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFuncFastKeywords)(PyObject *,
                                         PyObject *,
                                         PyObject *const *, Py_ssize_t,
                                         PyObject *);


/* ===== Flags for PyCFunctionDef.ml_flags ===== */

/* ----- Flags influencing __call__ ----- */

/* Exactly one the following must be given */
#define METH_VARARGS        0x0001
#define METH_FASTCALL       0x0080
#define METH_NOARGS         0x0004
#define METH_O              0x0008

/* Can be combined with either METH_VARARGS or METH_FASTCALL */
#define METH_KEYWORDS       0x0002

/* Pass the function object as additional first argument to the C
   function (PEP 575) */
#define METH_PASS_FUNCTION  0x0200

/* All flags which determine the signature of the C function */
#ifndef Py_LIMITED_API
#define METH_CALLFLAGS  (METH_VARARGS|METH_FASTCALL|METH_NOARGS|METH_O|METH_KEYWORDS|METH_PASS_FUNCTION)
#endif

/* Never pass args[0] as "self" argument (PEP 575) */
#define METH_CALL_UNBOUND   0x0800

/* ----- Flags influencing the creation of function objects ----- */

/* METH_STATIC and METH_CLASS control the construction of methods for
   a class. */
#define METH_STATIC         0x0020
#define METH_CLASS          0x0010

/* METH_COEXIST can only be used for methods of a class.
   It allows a method to be entered even though a slot has
   already filled the entry.  When defined, the flag allows a separate
   method, "__contains__" for example, to coexist with a defined
   slot like sq_contains. */
#define METH_COEXIST        0x0040

/* METH_BINDING can only be used for functions of a module, so we can
   safely re-use the constant METH_COEXIST.
   If set, do not set m_self to the definining module. The m_parent
   field is still set to the module (PEP 573; PEP 575). */
#define METH_BINDING        0x0040

/* If set, do not auto-generate a function at all (PEP 575) */
#define METH_CUSTOM         0x0400

/* ----- Other flags ----- */

/* This bit is reserved for Stackless Python */
#define METH_STACKLESS      0x0100

#ifndef Py_LIMITED_API
/* Function should be considered a Python function (PEP 575) */
#define METH_PYTHON         0x1000

/* User-defined flags */
#define METH_USR0         0x010000
#define METH_USR1         0x020000
#define METH_USR2         0x040000
#define METH_USR3         0x080000
#define METH_USR4         0x100000
#define METH_USR5         0x200000
#define METH_USR6         0x400000
#define METH_USR7         0x800000
#endif


typedef struct {
    const char *ml_name;   /* The name of the built-in function/method */
    PyCFunction ml_meth;   /* The C function that implements it */
    int         ml_flags;  /* Combination of METH_xxx flags, which mostly
                              describe the args expected by the C func */
} PyCFunctionDef;


#ifndef Py_LIMITED_API
typedef struct {
    PyObject_HEAD
    PyCFunctionDef *m_ml;     /* Description of the C function to call */
    PyObject *m_self;         /* __self__: anything, can be NULL; readonly */
    PyObject *m_module;       /* __module__: anything */
    PyObject *m_parent;       /* __parent__: anything or NULL; readonly */
    PyObject *m_weakreflist;  /* List of weak references */
} PyBaseFunctionObject;
#endif

PyAPI_DATA(PyTypeObject) PyBaseFunction_Type;

#define PyBaseFunction_CheckFast(op) \
        PyType_HasFeature(Py_TYPE(op), Py_TPFLAGS_BASEFUNCTION)
#define PyBaseFunction_Check(op) (PyBaseFunction_CheckFast(op) || \
                                  PyObject_TypeCheck(op, &PyBaseFunction_Type))

PyAPI_FUNC(PyObject *) PyBaseFunction_New(PyTypeObject *,
        PyCFunctionDef *, PyObject *, PyObject *, PyObject *);

PyAPI_FUNC(PyObject *) PyBaseFunction_Call(PyObject *, PyObject *, PyObject *);

PyAPI_FUNC(PyCFunction) PyBaseFunction_GetFunction(PyObject *);
PyAPI_FUNC(PyObject *) PyBaseFunction_GetSelf(PyObject *);
PyAPI_FUNC(int) PyBaseFunction_GetFlags(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#ifndef Py_LIMITED_API
#define PyBaseFunction_GET_FLAGS(func) ( \
        ((PyBaseFunctionObject *)func) -> m_ml -> ml_flags)
#define PyBaseFunction_HAS_FLAG(func, flag) ( \
        PyBaseFunction_GET_FLAGS(func) & (flag))
#define PyBaseFunction_GET_NAME(func) ( \
        ((PyBaseFunctionObject *)func) -> m_ml -> ml_name)
#define PyBaseFunction_GET_FUNCTION(func) ( \
        ((PyBaseFunctionObject *)func) -> m_ml -> ml_meth)
#define PyBaseFunction_REAL_SELF(func) ( \
        ((PyBaseFunctionObject *)func) -> m_self)
#define PyBaseFunction_GET_SELF(func) ( \
        PyBaseFunction_HAS_FLAG(func, METH_STATIC) ? \
        NULL : PyBaseFunction_REAL_SELF(func))
#define PyBaseFunction_GET_PARENT(func) ( \
        ((PyBaseFunctionObject *)func) -> m_parent)
#define __PyType_or_NULL(obj) (obj != NULL && PyType_Check(obj) ? \
                               (PyTypeObject *)(obj) : NULL)
#define PyBaseFunction_GET_OBJCLASS(func) \
        __PyType_or_NULL(PyBaseFunction_GET_PARENT(func))

PyAPI_FUNC(int) _PyBaseFunction_Clear(PyObject *);

PyAPI_FUNC(int) _PyBaseFunction_RichCompareBool(PyObject *, PyObject *, int);

PyAPI_FUNC(int)
_PyBaseFunction_CheckObjclass(PyObject *func, PyObject *arg);

#define _PyBaseFunction_SelfSlicing(func) ( \
        PyBaseFunction_REAL_SELF(func) == NULL && \
        !PyBaseFunction_HAS_FLAG(func, METH_CALL_UNBOUND))

PyAPI_FUNC(PyObject *) _PyBaseFunction_FastCall(
    PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *keywords);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_BASEFUNCTION_H */
