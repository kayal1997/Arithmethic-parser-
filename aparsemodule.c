#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "tinyexpr.h"


static PyObject *
aparse_parser(PyObject *self, PyObject *args)
{
    char *expression;
    char *output;
    int sts;

    if (!PyArg_ParseTuple(args, "s", &expression))
        return NULL;
	double x, y;
    te_variable vars[] = {{"x", &x}, {"y", &y}};

    /* This will compile the expression and check for errors. */
    int err;
	printf("expression = %s\n",expression);
    te_expr *n = te_compile(expression, vars, 2, &err);
	Rational r;
    if (n) {
        /* The variables can be changed here, and eval can be called as many
         * times as you like. This is fairly efficient because the parsing has
         * already been done. */
        x = 3; y = 4;
         r = te_eval(n);

        te_free(n);
    } else {
        /* Show the user where the error is at. */
       // printf("\t%*s^\nError near here", err-1, "");
    }	
	sprintf(output,"%lld/%lld",r.numerator,r.denominator);
    return PyUnicode_FromString(output);
}


static PyMethodDef aparseMethods[] = {
    {"parser",  aparse_parser, METH_VARARGS,
     "Execute a shell command."},
    {NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef aparsemodule = {
    PyModuleDef_HEAD_INIT,
    "aparse",   /* name of module */
    NULL, /* module documentation, may be NULL */
    -1,       /* size of per-interpreter state of the module,
                 or -1 if the module keeps state in global variables. */
    aparseMethods
};


static PyObject *aparseError;

PyMODINIT_FUNC
PyInit_aparse(void)
{
    PyObject *m;

    m = PyModule_Create(&aparsemodule);
    if (m == NULL)
        return NULL;

    aparseError = PyErr_NewException("aparse.error", NULL, NULL);
    Py_XINCREF(aparseError);
    if (PyModule_AddObject(m, "error", aparseError) < 0) {
        Py_XDECREF(aparseError);
        Py_CLEAR(aparseError);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

