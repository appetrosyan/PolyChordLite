#include <Python.h>
#include <stdexcept>
#include <iostream>
#include "interfaces.h"
#include "interfaces.hpp"

#if PY_MAJOR_VERSION >=3
#define PYTHON3
#endif

/* Exception */
class PythonException : std::exception {};


/* Docstrings */
static char module_docstring[] =
    "PyPolyChord: This module provides a Python interface to PolyChord.";
static char run_docstring[] =
    "Runs PyPolyChord";


/* Available functions */
static PyObject *run_PyPolyChord(PyObject *self, PyObject *args);


/* Module interface */
static PyMethodDef module_methods[] = {
    {"run", run_PyPolyChord, METH_VARARGS, run_docstring},
    {NULL, NULL, 0, NULL}
};

#ifdef PYTHON3
static struct PyModuleDef cModPyDem =
{
    PyModuleDef_HEAD_INIT,
    "_PyPolyChord", /* name of module */
    module_docstring,          /* module documentation, may be NULL */
    -1,          /* size of per-interpreter state of the module, or -1 if the module keeps state in global variables. */
    module_methods
};
#endif

/* Initialize the module */
#ifdef PYTHON3
PyMODINIT_FUNC PyInit__PyPolyChord(void)
{
    return PyModule_Create(&cModPyDem);
}
#else
PyMODINIT_FUNC init_PyPolyChord(void)
{
    Py_InitModule3("_PyPolyChord", module_methods, module_docstring);
}
#endif


/* Convert from C array to Python list */
PyObject* list_C2Py(double* array, int n) {
    PyObject* list = PyList_New(n);
    for (int i=0; i<PyList_Size(list); i++) 
        PyList_SET_ITEM(list, i, PyFloat_FromDouble(array[i]));
    return list;
}

/* Convert from Python list to C array */
void list_Py2C(PyObject* list, double* array) {
    int i;
    for (i=0; i<PyList_Size(list); i++) 
    {
        PyObject *obj = PyList_GET_ITEM(list, i);
        if (obj==NULL) throw PythonException();
        if(!PyFloat_Check(obj)) throw PythonException();
        array[i] = PyFloat_AsDouble(obj);
    }
}
void list_Py2C(PyObject* list, int* array) {
    int i;
    for (i=0; i<PyList_Size(list); i++) 
    {
        PyObject *obj = PyList_GET_ITEM(list, i);
        if (obj==NULL) throw PythonException();
#ifdef PYTHON3
        if(!PyLong_Check(obj)) throw PythonException();
        array[i] = PyLong_AsLong(obj);
#else
        if(PyInt_Check(obj)) array[i] = PyInt_AsLong(obj); 
        else if(PyLong_Check(obj)) array[i] = PyInt_AsLong(obj); 
        else throw PythonException();
#endif
    }
}

void dict_Py2C(PyObject* dict, double* loglikes, int* nlives) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;
    int i=0;
    if (dict==NULL) throw PythonException();

    while (PyDict_Next(dict, &pos, &key, &value)) {
        if (key==NULL) throw PythonException();
        if (value==NULL) throw PythonException();
#ifdef PYTHON3
        if(!PyLong_Check(value)) throw PythonException();
        nlives[i] = PyLong_AsLong(value);
#else
        if(PyInt_Check(value)) nlives[i] = PyInt_AsLong(value); 
        else if(PyLong_Check(value)) nlives[i] = PyInt_AsLong(value); 
        else throw PythonException();
#endif
        if(PyFloat_Check(key)) loglikes[i] = PyFloat_AsDouble(key);
        else throw PythonException();
        i++;
    }
}

/* Callback to the likelihood and prior */
static PyObject *python_loglikelihood = NULL;

double loglikelihood(double* theta, int nDims, double* phi, int nDerived)
{
    /* Create a python version of theta */
    PyObject *list_theta = list_C2Py(theta,nDims);
    if (list_theta==NULL) throw PythonException();

    /* Compute the likelihood and phi from theta  */
    PyObject* answer = PyObject_CallFunctionObjArgs(python_loglikelihood,list_theta,NULL);
    if (answer==NULL){
        Py_DECREF(list_theta);
        throw PythonException();
    }

    if (!PyTuple_Check(answer) or PyTuple_Size(answer) != 2){
        Py_DECREF(list_theta); Py_DECREF(answer);
        PyErr_SetString(PyExc_TypeError,"Return from loglikelihood must be a tuple of (loglikelihood, [derived parameters])");
        throw PythonException();
    }
    
    PyObject *py_logL = PyTuple_GetItem(answer,0);
    if (!PyFloat_Check(py_logL)){
        Py_DECREF(list_theta); Py_DECREF(answer);
        PyErr_SetString(PyExc_TypeError,"loglikelihood must be a float (element 0 of loglikelihood return)");
        throw PythonException();
    }
    double logL = PyFloat_AsDouble( PyTuple_GetItem(answer,0));

    PyObject *py_derived = PyTuple_GetItem(answer,1);
    if (!PyList_Check(py_derived)){
        Py_DECREF(list_theta); Py_DECREF(answer);
        PyErr_SetString(PyExc_TypeError,"Derived parameters must be a list (element 1 of loglikelihood return)");
        throw PythonException();
    } 
    else if(PyList_Size(py_derived) != nDerived)
    {
        Py_DECREF(list_theta); Py_DECREF(answer);
        PyErr_SetString(PyExc_ValueError,"Derived parameters must have length nDerived (element 1 of loglikelihood return)");
        throw PythonException();
    }

    try{ list_Py2C(PyTuple_GetItem(answer,1),phi); }
    catch (PythonException& e) {
        PyErr_SetString(PyExc_TypeError,"Derived parameters must be a list of floats (element 1 of loglikelihood return)");
        throw PythonException(); }

    /* Garbage collect */
    Py_DECREF(list_theta);
    Py_DECREF(answer);

    return logL;
}

static PyObject *python_prior = NULL;

void prior(double* cube, double* theta, int nDims)
{
    /* create a python version of cube */
    PyObject* list_cube = list_C2Py(cube,nDims);
    if (list_cube==NULL) throw PythonException();

    /* Compute theta from the prior */
    PyObject* list_theta = PyObject_CallFunctionObjArgs(python_prior,list_cube,NULL);
    if (list_theta==NULL) {Py_DECREF(list_cube); throw PythonException();}

    if (!PyList_Check(list_theta)){
        Py_DECREF(list_theta); Py_DECREF(list_cube);
        PyErr_SetString(PyExc_TypeError,"Physical parameters must be a list (return from prior)");
        throw PythonException();
    } 
    else if(PyList_Size(list_theta) != nDims)
    {
        Py_DECREF(list_theta); Py_DECREF(list_cube);
        PyErr_SetString(PyExc_ValueError,"Physical parameters must have length nDims (return from prior)");
        throw PythonException();
    }

    /* Convert the python answer back to a C array */
    try{ list_Py2C(list_theta,theta); }
    catch (PythonException& e){
        Py_DECREF(list_theta); Py_DECREF(list_cube);
        PyErr_SetString(PyExc_TypeError,"Physical parameters must be a list of floats (return from prior)");
        throw PythonException(); 
    }

    /* Garbage collect */
    Py_DECREF(list_cube);
    Py_DECREF(list_theta);
}


/* Function to run PyPolyChord */
static PyObject *run_PyPolyChord(PyObject *, PyObject *args)
{
    Settings S;

    PyObject *temp_logl, *temp_prior;
    PyObject* py_grade_dims, *py_grade_frac, *py_nlives;
    char* base_dir, *file_root;
        

    if (!PyArg_ParseTuple(args,
                "OOiiiiiiididiiiiiiiiiidssO!O!O!i:run",
                &temp_logl,
                &temp_prior,
                &S.nDims,
                &S.nDerived,
                &S.nlive,
                &S.num_repeats,
                &S.nprior,
                &S.do_clustering,
                &S.feedback,
                &S.precision_criterion,
                &S.max_ndead,
                &S.boost_posterior,
                &S.posteriors,
                &S.equals,
                &S.cluster_posteriors,
                &S.write_resume,
                &S.write_paramnames,
                &S.read_resume,
                &S.write_stats,
                &S.write_live,
                &S.write_dead,
                &S.write_prior,
                &S.compression_factor,
                &base_dir,
                &file_root,
                &PyList_Type,
                &py_grade_frac,
                &PyList_Type,
                &py_grade_dims,
                &PyDict_Type,
                &py_nlives,
                &S.seed
                )
            )
        return NULL;
    S.base_dir = base_dir;
    S.file_root = file_root;
    
    int nGrade = PyList_Size(py_grade_frac);
    double grade_frac[nGrade];
    int grade_dims[nGrade];
    Py_INCREF(py_grade_frac); Py_INCREF(py_grade_dims);

    int n_nlives = PyDict_Size(py_nlives);
    double loglikes[n_nlives];
    int nlives[n_nlives];
    Py_INCREF(py_nlives);

    try{ list_Py2C(py_grade_frac,grade_frac); }
    catch (PythonException& e){
        Py_DECREF(py_grade_frac); Py_DECREF(py_grade_dims);
        PyErr_SetString(PyExc_TypeError,"grade_frac must be a list of doubles");
        return NULL;
    }
    try{ list_Py2C(py_grade_dims,grade_dims); }
    catch (PythonException& e){
        Py_DECREF(py_grade_frac); Py_DECREF(py_grade_dims);
        PyErr_SetString(PyExc_TypeError,"grade_dims must be a list of integers");
        return NULL;
    }
    if (PyList_Size(py_grade_frac) != PyList_Size(py_grade_dims)){
        Py_DECREF(py_grade_frac); Py_DECREF(py_grade_dims);
        PyErr_SetString(PyExc_ValueError,"grade_dims and grade_frac must have the same size");
        return NULL;
    }
    int tot = 0; for (int i=0;i<nGrade;i++) tot += grade_dims[i];
    if (tot != S.nDims) {
        PyErr_SetString(PyExc_ValueError,"grade_dims must sum to nDims");
        return NULL;
    }
    try{ dict_Py2C(py_nlives,loglikes,nlives); }
    catch (PythonException& e){
        Py_DECREF(py_nlives);
        PyErr_SetString(PyExc_TypeError,"nlives must be a dict mapping floats to integers");
        return NULL;
    }

    Py_XINCREF(temp_logl);
    Py_XDECREF(python_loglikelihood);
    python_loglikelihood = temp_logl;

    Py_XINCREF(temp_prior);
    Py_XDECREF(python_prior);
    python_prior = temp_prior;

    /* Run PolyChord */
    try{ run_polychord(loglikelihood, prior, S); }
    catch (PythonException& e)
    { 
        Py_DECREF(py_grade_frac);Py_DECREF(py_grade_dims);Py_DECREF(python_loglikelihood);Py_DECREF(python_prior);
        return NULL; 
    }

    /* Return None */
    Py_RETURN_NONE;
}

