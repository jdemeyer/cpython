/* Built-in function interface */

#ifndef Py_CFUNCTION_H
#define Py_CFUNCTION_H
#ifdef __cplusplus
extern "C" {
#endif


PyAPI_DATA(PyTypeObject) PyCFunction_Type;

#define PyCFunction_Check(op) (Py_TYPE(op) == &PyCFunction_Type)

struct PyMethodDef {
    const char  *ml_name;   /* The name of the built-in function/method */
    PyCFunction ml_meth;    /* The C function that implements it */
    int         ml_flags;   /* Combination of METH_xxx flags, which mostly
                               describe the args expected by the C func */
    const char  *ml_doc;    /* The __doc__ attribute, or NULL */
};
typedef struct PyMethodDef PyMethodDef;


#ifndef Py_LIMITED_API
/* Same as PyBaseFunctionObject but type of m_ml is (PyMethodDef *) */
typedef struct {
    PyObject_HEAD
    PyMethodDef *m_ml;        /* Description of the C function to call */
    PyObject *m_self;         /* __self__: anything, can be NULL; readonly */
    PyObject *m_module;       /* __module__: anything */
    PyObject *m_parent;       /* __parent__: anything or NULL; readonly */
    PyObject *m_weakreflist;  /* List of weak references */
} PyCFunctionObject;
#endif

PyAPI_FUNC(PyObject *) PyCFunction_NewEx(PyMethodDef *, PyObject *,
                                         PyObject *);


/* Backwards compatibility (PEP 575) */
#ifndef Py_LIMITED_API
#define PyCFunction_New(ml, self) PyCFunction_NewEx(ml, self, NULL)

#define PyCFunction_GetFunction PyBaseFunction_GetFunction
#define PyCFunction_GetSelf PyBaseFunction_GetSelf
#define PyCFunction_GetFlags PyBaseFunction_GetFlags

#define PyCFunction_GET_FUNCTION PyBaseFunction_GET_FUNCTION
#define PyCFunction_GET_SELF PyBaseFunction_GET_SELF
#define PyCFunction_GET_FLAGS PyBaseFunction_GET_FLAGS

#define PyCFunction_Call PyBaseFunction_Call

PyAPI_FUNC(PyObject *) _PyMethodDef_RawFastCallDict(
    PyMethodDef *method,
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwargs);

PyAPI_FUNC(PyObject *) _PyMethodDef_RawFastCallKeywords(
    PyMethodDef *method,
    PyObject *self,
    PyObject *const *args,
    Py_ssize_t nargs,
    PyObject *kwnames);

#define _PyCFunctionFast PyCFunctionFast
#define _PyCFunctionFastWithKeywords PyCFunctionFastKeywords

#endif

typedef PyObject *(*PyNoArgsFunction)(PyObject *);


#ifdef __cplusplus
}
#endif
#endif /* !Py_CFUNCTION_H */
