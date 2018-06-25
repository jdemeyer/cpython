
/* Method object interface */

#ifndef Py_METHODOBJECT_H
#define Py_METHODOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


/* This is about the classes 'builtin_function_or_method'
   and 'method_descriptor', not Python methods in user-defined
   classes.  See classobject.h for the latter. */

PyAPI_DATA(PyTypeObject) PyCFunction_Type;
PyAPI_DATA(PyTypeObject) PyMethodDescr_Type;

#define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

typedef PyObject *(*PyCFunction)(PyObject *, PyObject *);
typedef PyObject *(*_PyCFunctionFast) (PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunctionWithKeywords)(PyObject *, PyObject *,
                                             PyObject *);
typedef PyObject *(*_PyCFunctionFastWithKeywords) (PyObject *,
                                                   PyObject *const *, Py_ssize_t,
                                                   PyObject *);
typedef PyObject *(*PyNoArgsFunction)(PyObject *);

PyAPI_FUNC(PyCFunction) PyCFunction_GetFunction(PyObject *);
PyAPI_FUNC(PyObject *) PyCFunction_GetSelf(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#ifndef Py_LIMITED_API
#define PyCFunction_GET_FUNCTION(func) ( \
        (PyCFunction)((PyCFunctionObject *)func)->m_ccall->cc_func)
#define PyCFunction_GET_SELF(func) ( \
        (((PyCFunctionObject *)func)->m_self == Py_None) ? \
        NULL : ((PyCFunctionObject *)func)->m_self)
#endif
PyAPI_FUNC(PyObject *) PyCFunction_Call(PyObject *, PyObject *, PyObject *);

struct PyMethodDef {
    const char  *ml_name;   /* The name of the built-in function/method */
    PyCFunction ml_meth;    /* The C function that implements it */
    int         ml_flags;   /* Combination of METH_xxx flags, which mostly
                               describe the args expected by the C func */
    const char  *ml_doc;    /* The __doc__ attribute, or NULL */
};
typedef struct PyMethodDef PyMethodDef;

#define PyCFunction_New(ML, SELF) PyCFunction_NewEx((ML), (SELF), NULL)
PyAPI_FUNC(PyObject *) PyCFunction_NewEx(PyMethodDef *, PyObject *,
                                         PyObject *);

/* Flag passed to newmethodobject */
/* #define METH_OLDARGS  0x0000   -- unsupported now */
#define METH_VARARGS  0x0001
#define METH_KEYWORDS 0x0002
/* METH_NOARGS and METH_O must not be combined with the flags above. */
#define METH_NOARGS   0x0004
#define METH_O        0x0008

/* METH_CLASS and METH_STATIC are a little different; these control
   the construction of methods for a class.  These cannot be used for
   functions in modules. */
#define METH_CLASS    0x0010
#define METH_STATIC   0x0020

/* METH_COEXIST allows a method to be entered even though a slot has
   already filled the entry.  When defined, the flag allows a separate
   method, "__contains__" for example, to coexist with a defined
   slot like sq_contains. */

#define METH_COEXIST   0x0040

#ifndef Py_LIMITED_API
#define METH_FASTCALL  0x0080

/* All flags influencing the signature of the C function */
#define METH_SIGNATURE (METH_VARARGS | METH_FASTCALL | METH_NOARGS | METH_O | METH_KEYWORDS)
#endif

/* This bit is preserved for Stackless Python */
#ifdef STACKLESS
#define METH_STACKLESS 0x0100
#else
#define METH_STACKLESS 0x0000
#endif

PyAPI_FUNC(PyObject *) PyDescr_NewMethod(PyTypeObject *, PyMethodDef *);

PyAPI_FUNC(PyObject *) PyCFunction_ClsNew(
    PyTypeObject *cls,
    PyMethodDef *ml,
    PyObject *self,
    PyObject *module,
    PyObject *parent);


/* C call protocol (PEP 580) */

#ifndef Py_LIMITED_API
/* Various function pointer types used for cc_func. The letters indicate
   the kind of argument:
     F: function object
     S: __self__
     A: *args or single argument
     N: C array for METH_FASTCALL
     n: len(args) for METH_FASTCALL
     K: **kwargs
   */
typedef PyObject *(*PyCFunc_SA)(PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_SAK)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_SNn)(PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunc_SNnK)(PyObject *, PyObject *const *, Py_ssize_t, PyObject *);
typedef PyObject *(*PyCFunc_FSA)(PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_FSAK)(PyObject *, PyObject *, PyObject *, PyObject *);
typedef PyObject *(*PyCFunc_FSNn)(PyObject *, PyObject *, PyObject *const *, Py_ssize_t);
typedef PyObject *(*PyCFunc_FSNnK)(PyObject *, PyObject *, PyObject *const *, Py_ssize_t, PyObject *);

/* Unspecified function pointer */
typedef void *(*PyCFunc)(void);

typedef struct {
    uint32_t     cc_flags;
    PyCFunc      cc_func;    /* C function to call */
    PyObject    *cc_name;    /* str object */
    PyObject    *cc_parent;  /* class or module or anything, may be NULL */
} PyCCallDef;

PyAPI_FUNC(int) _PyCCallDef_FromMethodDef(PyCCallDef *cc,
                                          PyMethodDef *ml, PyObject *parent);

/* Base signature: one of 4 possibilities, exactly one must be given.
   The rationale for the numerical values is as follows:
   - the valid combinations with CCALL_KEYWORDS | CCALL_FUNCARG give
     precisely the range [0, ..., 11]. This allows the compiler to
     implement the main switch statement in PyCCall_FASTCALL() using a
     jump table.
   - in a few places we are explicitly checking for CCALL_VARARGS,
     which is fastest when CCALL_VARARGS == 0
*/
#define CCALL_VARARGS          0x00000000
#define CCALL_FASTCALL         0x00000002
#define CCALL_O                0x00000004
#define CCALL_NULLARG          0x00000006

/* Other flags, these are single bits */
#define CCALL_KEYWORDS         0x00000008
#define CCALL_FUNCARG          0x00000001
#define CCALL_OBJCLASS         0x00000010
#define CCALL_PROFILE          0x00000020
#define CCALL_SLICE_SELF       0x00000040

/* Combinations of some of the above flags */
#define CCALL_BASESIGNATURE    (CCALL_VARARGS | CCALL_FASTCALL | CCALL_O | CCALL_NULLARG)
#define CCALL_SIGNATURE        (CCALL_BASESIGNATURE | CCALL_KEYWORDS | CCALL_FUNCARG)

/* Hack to add a special error message for print >> f */
#define _CCALL_BUILTIN_PRINT   0x10000000

#define PyCCall_Check(op) (Py_TYPE(op)->tp_flags & Py_TPFLAGS_HAVE_CCALL)

typedef struct {
    PyCCallDef *cr_ccall;
    PyObject   *cr_self;     /* __self__ argument for methods */
} PyCCallRoot;

PyAPI_FUNC(PyObject *) PyCCall_Call(PyObject *func, PyObject *args, PyObject *kwds);
PyAPI_FUNC(PyObject *) PyCCall_FASTCALL(
    PyObject *func,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *keywords);

#define PyCCall_CCALLROOT(func) ((PyCCallRoot*)(((char*)func) + Py_TYPE(func)->tp_ccalloffset))
#define PyCCall_CCALLDEF(func) (PyCCall_CCALLROOT(func)->cr_ccall)
#define PyCCall_SELF(func) (PyCCall_CCALLROOT(func)->cr_self)

PyAPI_FUNC(PyObject *) PyCCall_GenericGetSelf(PyObject *, void *);
PyAPI_FUNC(PyObject *) PyCCall_GenericGetName(PyObject *, void *);
PyAPI_FUNC(PyObject *) PyCCall_GenericGetQualname(PyObject *, void *);
PyAPI_FUNC(PyObject *) PyCCall_GenericGetParent(PyObject *, void *);
#endif   /* Py_LIMITED_API */


#ifndef Py_LIMITED_API
typedef struct {
    PyObject_HEAD
    PyCCallDef  *m_ccall;
    PyObject    *m_self;   /* Passed as 'self' arg to the C func */
    PyCCallDef   _ccalldef;
    PyObject    *m_module; /* The __module__ attribute, can be anything */
    const char  *m_doc;    /* __text_signature__ and __doc__ */
    PyObject    *m_weakreflist; /* List of weak references */
} PyCFunctionObject;

PyAPI_FUNC(PyObject *) _PyCFunction_NewBoundMethod(PyCFunctionObject *func, PyObject *self);
#endif

PyAPI_FUNC(int) PyCFunction_ClearFreeList(void);

#ifndef Py_LIMITED_API
PyAPI_FUNC(void) _PyCFunction_DebugMallocStats(FILE *out);
PyAPI_FUNC(void) _PyMethod_DebugMallocStats(FILE *out);
#endif

#ifdef __cplusplus
}
#endif
#endif /* !Py_METHODOBJECT_H */
