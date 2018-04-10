
/* Function object interface */
#ifndef Py_LIMITED_API
#ifndef Py_FUNCOBJECT_H
#define Py_FUNCOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif


typedef struct {
    PyBaseFunctionObject base;
    PyObject *func_dict;        /* __dict__: dict or NULL */
} PyDefinedFunctionObject;

PyAPI_DATA(PyTypeObject) PyDefinedFunction_Type;

#define PyDefinedFunction_Check(op) (PyObject_TypeCheck(op, &PyDefinedFunction_Type))


/* Function objects and code objects should not be confused with each other:
 *
 * Function objects are created by the execution of the 'def' statement.
 * They reference a code object in their __code__ attribute, which is a
 * purely syntactic object, i.e. nothing more than a compiled version of some
 * source code lines.  There is one code object per source code "fragment",
 * but each code object can be referenced by zero or many function objects
 * depending only on how many times the 'def' statement in the source was
 * executed so far.
 */

/* Invariant:
 *     func_closure contains the bindings for func_code->co_freevars, so
 *     PyTuple_Size(func_closure) == PyCode_GetNumFree(func_code)
 *     (func_closure may be NULL if PyCode_GetNumFree(func_code) == 0).
 */

typedef struct {
    PyBaseFunctionObject base;
    PyObject *func_dict;        /* __dict__: dict or NULL */
    PyObject *func_name;        /* __name__: string */
    PyObject *func_qualname;    /* __qualname__: string */
    PyObject *func_code;        /* __code__: code */
    PyObject *func_globals;     /* __globals__: anything; readonly */
    PyObject *func_doc;         /* __doc__: can be anything or NULL */
    PyObject *func_defaults;    /* __defaults__: tuple or NULL */
    PyObject *func_kwdefaults;  /* __kwdefaults__: dict or NULL */
    PyObject *func_closure;     /* __closure__: tuple of cell objects or NULL; readonly */
    PyObject *func_annotations; /* __annotations__: dict or NULL */
    PyCFunctionDef _ml;         /* Storage for base.m_ml */
} PyFunctionObject;

PyAPI_DATA(PyTypeObject) PyFunction_Type;

#define PyFunction_Check(op) (PyBaseFunction_Check(op) && \
                              PyBaseFunction_HAS_FLAG(op, METH_PYTHON))
#define PyFunction_CheckFast(op) (PyBaseFunction_CheckFast(op) && \
                                  PyBaseFunction_HAS_FLAG(op, METH_PYTHON))
#define PyFunction_CheckExact(op) (Py_TYPE(op) == &PyFunction_Type)

#define PyFunction_New(code, globals) \
        PyFunction_NewPython(&PyFunction_Type, code, globals, NULL, NULL)
#define PyFunction_NewWithQualName(code, globals, qualname) \
        PyFunction_NewPython(&PyFunction_Type, code, globals, NULL, qualname)
PyAPI_FUNC(PyObject *) PyFunction_NewPython(
        PyTypeObject *, PyObject *, PyObject *, PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_Copy(PyTypeObject *, PyObject *);

PyAPI_FUNC(PyObject *) PyFunction_GetCode(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetGlobals(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetModule(PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetKwDefaults(PyObject *);
PyAPI_FUNC(int) PyFunction_SetKwDefaults(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetClosure(PyObject *);
PyAPI_FUNC(int) PyFunction_SetClosure(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyFunction_GetAnnotations(PyObject *);
PyAPI_FUNC(int) PyFunction_SetAnnotations(PyObject *, PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyFunction_GET_CODE(func) \
        (((PyFunctionObject *)func) -> func_code)
#define PyFunction_GET_GLOBALS(func) \
        (((PyFunctionObject *)func) -> func_globals)
#define PyFunction_GET_MODULE(func) \
        (((PyBaseFunctionObject *)func) -> m_module)
#define PyFunction_GET_DEFAULTS(func) \
        (((PyFunctionObject *)func) -> func_defaults)
#define PyFunction_GET_KW_DEFAULTS(func) \
        (((PyFunctionObject *)func) -> func_kwdefaults)
#define PyFunction_GET_CLOSURE(func) \
        (((PyFunctionObject *)func) -> func_closure)
#define PyFunction_GET_ANNOTATIONS(func) \
        (((PyFunctionObject *)func) -> func_annotations)

#define _PyFunction_FastCallKeywords(func, stack, nargs, kwnames) \
        _PyFunction_FastCallSelf(func, NULL, stack, nargs, kwnames)

PyAPI_FUNC(PyObject *) _PyFunction_FastCallSelf(
    PyObject *func,
    PyObject *self,
    PyObject *const *stack,
    Py_ssize_t nargs,
    PyObject *kwnames);

/* The classmethod and staticmethod types lives here, too */
PyAPI_DATA(PyTypeObject) PyClassMethod_Type;
PyAPI_DATA(PyTypeObject) PyStaticMethod_Type;

PyAPI_FUNC(PyObject *) PyClassMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyStaticMethod_New(PyObject *);

#ifdef __cplusplus
}
#endif
#endif /* !Py_FUNCOBJECT_H */
#endif /* Py_LIMITED_API */
