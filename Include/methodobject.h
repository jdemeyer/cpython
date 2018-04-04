/* bound_method object interface */

#ifndef Py_LIMITED_API
#ifndef Py_METHODOBJECT_H
#define Py_METHODOBJECT_H
#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    PyBaseFunctionObject base;
    PyObject *im_func;  /* __func__: callable implementing the method; readonly */
} PyMethodObject;

PyAPI_DATA(PyTypeObject) PyMethod_Type;

#define PyMethod_Check(op) ((op)->ob_type == &PyMethod_Type)

PyAPI_FUNC(PyObject *) PyMethod_New(PyObject *, PyObject *);
PyAPI_FUNC(PyObject *) PyMethod_Function(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyMethod_GET_FUNCTION(meth) \
        (((PyMethodObject *)meth) -> im_func)

PyAPI_FUNC(int) PyMethod_ClearFreeList(void);

PyAPI_FUNC(void) _PyMethod_DebugMallocStats(FILE *out);

/* Backwards compatibility (PEP 575) */
#define PyMethod_Self PyBaseFunction_GetSelf
#define PyMethod_GET_SELF PyBaseFunction_REAL_SELF


typedef struct {
    PyObject_HEAD
    PyObject *func;
} PyInstanceMethodObject;

PyAPI_DATA(PyTypeObject) PyInstanceMethod_Type;

#define PyInstanceMethod_Check(op) ((op)->ob_type == &PyInstanceMethod_Type)

PyAPI_FUNC(PyObject *) PyInstanceMethod_New(PyObject *);
PyAPI_FUNC(PyObject *) PyInstanceMethod_Function(PyObject *);

/* Macros for direct access to these values. Type checks are *not*
   done, so use with care. */
#define PyInstanceMethod_GET_FUNCTION(meth) \
        (((PyInstanceMethodObject *)meth) -> func)

#ifdef __cplusplus
}
#endif
#endif /* !Py_METHODOBJECT_H */
#endif /* Py_LIMITED_API */
