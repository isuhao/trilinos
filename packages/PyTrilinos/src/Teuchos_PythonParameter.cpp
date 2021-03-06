// @HEADER
// ***********************************************************************
//
//              PyTrilinos: Python Interface to Trilinos
//                 Copyright (2005) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// This library is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as
// published by the Free Software Foundation; either version 2.1 of the
// License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
// USA
// Questions? Contact Bill Spotz (wfspotz@sandia.gov)
//
// ***********************************************************************
// @HEADER

// Include files
#include "Teuchos_PythonParameter.h"
#include "swigpyrun.h"

// Backward compatibility for python < 2.5
#if (PY_VERSION_HEX < 0x02050000)
typedef int Py_ssize_t;
#endif

// ****************************************************************** //

namespace Teuchos
{

// ****************************************************************** //

bool setPythonParameter(ParameterList     & plist,
			const std::string & name,
			PyObject          * value)
{
  static swig_type_info * swig_TPL_ptr   = SWIG_TypeQuery("Teuchos::ParameterList *");
  void * argp;

  // Boolean values
  if (PyBool_Check(value))
  {
    if (value == Py_True) plist.set(name,true );
    else                  plist.set(name,false);
  }
  // Integer values
  else if (PyInt_Check(value))
  {
    plist.set(name, (int)PyInt_AsLong(value));
  }
  // Floating point values
  else if (PyFloat_Check(value))
  {
    plist.set(name, PyFloat_AsDouble(value));
  }
  // String values
  else if (PyString_Check(value))
  {
    plist.set(name, std::string(PyString_AsString(value)));
  }
  // Dictionary values
  else if (PyDict_Check(value))
  {  
    // Convert the python dictionary to a ParameterList
    ParameterList * sublist = pyDictToNewParameterList(value);

    // Store the ParameterList
    plist.set(name,*sublist);
    delete sublist;
  }

  // None object not allowed: this is a python type not usable by
  // Trilinos solver packages, so we reserve it for the
  // getPythonParameter() function to indicate that the requested
  // parameter does not exist in the given ParameterList
  else if (value == Py_None)
  {
    return false;
  }

  // ParameterList values
  else if (SWIG_CheckState(SWIG_Python_ConvertPtr(value, &argp, swig_TPL_ptr, 0)))
  {
    ParameterList *arg = reinterpret_cast<ParameterList *>(argp);
    plist.set(name, *arg);
  }
  // All other value types are unsupported
  else
  {
    return false;
  }

  // Successful type conversion
  return true;
}    // setPythonParameter

// **************************************************************** //

PyObject * getPythonParameter(const ParameterList & plist,
			      const std::string   & name)
{
  static swig_type_info * swig_TPL_ptr = SWIG_TypeQuery("Teuchos::ParameterList *");

  // If parameter does not exist, return None
  if (!plist.isParameter(name)) return Py_BuildValue("");

  // Get the parameter entry.  I now deal with the ParameterEntry
  // objects so that I can query the ParameterList without setting
  // the used flag to true.
  const ParameterEntry * entry = plist.getEntryPtr(name);

  // Boolean parameter values
  if (entry->isType<bool>())
  {
    bool value = any_cast<bool>(entry->getAny(false));
    return PyBool_FromLong((long)value);
  }
  // Integer parameter values
  else if (entry->isType<int>())
  {
    int value = any_cast<int>(entry->getAny(false));
    return PyInt_FromLong((long)value);
  }
  // Double parameter values
  else if (entry->isType<double>())
  {
    double value = any_cast<double>(entry->getAny(false));
    return PyFloat_FromDouble(value);
  }
  // String parameter values
  else if (entry->isType<std::string>())
  {
    std::string value = any_cast<std::string>(entry->getAny(false));
    return PyString_FromString(value.c_str());
  }
  // Char * parameter values
  else if (entry->isType<char *>())
  {
    char * value = any_cast<char *>(entry->getAny(false));
    return PyString_FromString(value);
  }
  // ParameterList values
  else if (entry->isList())
  {
    const ParameterList & value = getValue<ParameterList>(*entry);
    return SWIG_NewPointerObj((void*) &value, swig_TPL_ptr, 0);
  }
  // All  other types are unsupported
  return NULL;
}    // getPythonParameter

// **************************************************************** //

bool isEquivalent(PyObject * dict, const ParameterList & plist)
{
  PyObject * key   = NULL;
  PyObject * value = NULL;
  PyObject * param = NULL;
  Py_ssize_t pos   = 0;
  string     name;

  // The dict pointer must point to a dictionary
  if (!PyDict_Check(dict)) goto fail;

  // Check that all entries in ParameterList are also in the
  // python dictionary
  for (ParameterList::ConstIterator i = plist.begin(); i != plist.end(); i++)
  {
    name  = plist.name(i);
    value = PyDict_GetItemString(dict,name.c_str());
    if (value == NULL) goto fail;
    if (plist.isSublist(name))
    {
      if (!isEquivalent(value, plist.sublist(name)))
	goto fail;
    }
    else
    {
      param = getPythonParameter(plist,name.c_str());
      if (param == NULL) goto fail;
      if (PyObject_RichCompareBool(param,value,Py_EQ) < 1) goto fail;
      Py_DECREF(param);
    }
  }
  // Check that all entries in the python dictionary are also in
  // the ParameterList
  while (PyDict_Next(dict, &pos, &key, &value))
  {
    if (!PyString_Check(key)) goto fail;
    name = string(PyString_AsString(key));
    if (!plist.isParameter(name)) goto fail;
    if (plist.isSublist(name))
    {
      if (!isEquivalent(value, plist.sublist(name)))
	goto fail;
    }
    else
    {
      param = getPythonParameter(plist, name);
      if (param == NULL) goto fail;
      if (PyObject_RichCompareBool(param,value,Py_EQ) < 1) goto fail;
      Py_DECREF(param);
    }
  }
  // All checks passed
  return true;

  fail:
  Py_XDECREF(param);
  return false;
}    // isEquivalent

// **************************************************************** //

bool updatePyDictWithParameterList(PyObject * dict, const ParameterList & plist,
				   ResponseToIllegalParameters flag)
{
  static char  illegalParam[ ] = "Illegal Parameters";
  PyObject   * value   = NULL;
  PyObject   * param   = NULL;
  bool         result  = true;
  const char * nameStr = NULL;
  string       name;

  // The dict pointer must point to a dictionary
  if (!PyDict_Check(dict))
  {
    PyErr_SetString(PyExc_TypeError, "Expected a dictionary");
    goto fail;
  }

  // Iterate over all entries in ParameterList and ensure they are
  // mirrored in the python dictionary
  for (ParameterList::ConstIterator i = plist.begin(); i != plist.end(); i++)
  {
    name    = plist.name(i);
    nameStr = name.c_str();
    param   = getPythonParameter(plist,nameStr);
    value   = PyDict_GetItemString(dict,nameStr);

    // If param is NULL, then behavior is determined by flag
    if (param == NULL)
    {
      switch (flag)
      {
      case ignore:
	break;
      case storeNames:
	result = false;
	value = PyDict_GetItemString(dict, illegalParam);
	if (value == NULL)
	{
	  PyDict_SetItemString(dict, illegalParam, Py_BuildValue("(s)", nameStr));
	}
	else
	{
	  if (!PyTuple_Check(value))
	  {
	    PyErr_Format(PyExc_ValueError, "Parameter '%s' has unexpected type",
			 illegalParam);
	    goto fail;
	  }
	  PyDict_SetItemString(dict, illegalParam,
			       PySequence_Concat(value, Py_BuildValue("(s)", nameStr)));
	}
	break;
      case raiseError:
	PyErr_Format(PyExc_ValueError, "Parameter '%s' is of unsupported type", nameStr);
	goto fail;
      default:
	PyErr_Format(PyExc_RuntimeError, "Unexpected enumeration encountered");
	goto fail;
      }
    }
    else
    {
      // If param is a sublist, mirror with a dictionary by calling
      // this routine recursively
      if (plist.isSublist(name))
      {
	if (value == NULL) value = PyDict_New();
	else if (!PyDict_Check(value))
	{
	  Py_DECREF(value);
	  value = PyDict_New();
	}
	result = result and 
	  updatePyDictWithParameterList(value, plist.sublist(name));
	PyDict_SetItemString(dict,nameStr,value);
      }

      // Else synchronize the dictionary value to the parameter
      else PyDict_SetItemString(dict,nameStr,param);
    }
    Py_XDECREF(param);
  }
  return result;
  fail:
  return false;
}    // updatePyDictWithParameterList

// **************************************************************** //

bool updateParameterListWithPyDict(PyObject * dict, ParameterList & plist,
				   ResponseToIllegalParameters flag)
{
  static char illegalKey[ ] = "Illegal Keys";
  PyObject *  key    = NULL;
  PyObject *  value  = NULL;
  Py_ssize_t  pos    = 0;
  bool        result = true;
  string      name;

  // The dict pointer must point to a dictionary
  if (!PyDict_Check(dict))
  {
    PyErr_SetString(PyExc_TypeError, "Expected a dictionary");
    goto fail;
  }

  // Iterate over all items in the python dictionary and ensure they
  // are synchronized with the ParameterList
  while (PyDict_Next(dict, &pos, &key, &value))
  {

    // If the key is not a string, we can't synchronize
    if (!PyString_Check(key))
    {
      PyErr_SetString(PyExc_TypeError, "Encountered non-string key in dictionary");
      goto fail;
    }

    name = string(PyString_AsString(key));
    if (!setPythonParameter(plist, name, value))
    {
      // If value is not settable, behavior is determined by flag
      switch (flag)
      {
      case ignore:
	break;
      case storeNames:
	result = false;
	if (plist.isParameter(illegalKey))
	{
	  const ParameterEntry * entry = plist.getEntryPtr(name);
	  if (entry->isType<string>())
	  {
	    string names = any_cast<string>(entry->getAny(false));
	    plist.set(illegalKey, names + string(", ") + name);
	  }
	  else
	  {
	    PyErr_Format(PyExc_TypeError, "Parameter '%s' has unexpected type",
			 illegalKey);
	    goto fail;
	  }
	}
	else
	{
	  plist.set(illegalKey, name);
	}
	break;
      case raiseError:
	PyErr_Format(PyExc_ValueError, "Parameter '%s' has unsupported value",
		     illegalKey);
	goto fail;
      default:
	PyErr_Format(PyExc_RuntimeError, "Unexpected enumeration encountered");
	goto fail;
      }
    }
  }
  return result;
  fail:
  return false;
}    // updateParameterListWithPyDict

// **************************************************************** //

bool synchronizeParameters(PyObject * dict, ParameterList & plist,
			   ResponseToIllegalParameters flag)
{
  bool result = true;
  result = result && updatePyDictWithParameterList(dict,plist,flag);
  result = result && updateParameterListWithPyDict(dict,plist,flag);
  return result;
}    // synchronizeParameters

// **************************************************************** //

Teuchos::ParameterList *
pyDictToNewParameterList(PyObject * dict,
			 ResponseToIllegalParameters flag)
{
  Teuchos::ParameterList * plist = 0;
  // The dict pointer must point to a dictionary
  if (!PyDict_Check(dict))
  {
    PyErr_SetString(PyExc_ValueError, "Expected a dictionary");
    goto fail;
  }

  // Create a new ParameterList and synchronize it with the python
  // dictionary
  plist = new ParameterList();
  if (!updateParameterListWithPyDict(dict,*plist,flag))
  {
    // If update failed, behavior is determined by flag
    switch (flag)
    {
    case ignore:
    case storeNames:
      break;
    case raiseError:
      delete plist;
      goto fail;
    default:
      delete plist;
      goto fail;
    }
  }
  return plist;
  fail:
  return NULL;
}    // pyDictToNewParameterList

// **************************************************************** //

PyObject * parameterListToNewPyDict(const ParameterList & plist,
				    ResponseToIllegalParameters flag)
{

  // Create a new dictionary and synchronize it with the ParameterList
  PyObject * dict = PyDict_New();
  if (!updatePyDictWithParameterList(dict,plist))
  {

    // If update failed, behavior is determined by flag
    switch (flag)
    {
    case ignore:
    case storeNames:
      break;
    case raiseError:
    default:
      Py_XDECREF(dict);
      goto fail;
    }
  }
  return dict;
  fail:
  return NULL;
}    // parameterListToNewPyDict

}    // namespace Teuchos
