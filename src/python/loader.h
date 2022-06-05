#pragma once
#include <string>
#include <tuple>
#include <typeindex>
#include <typeinfo>
#include <vector>
#include <deque>
#include <filesystem>
#include "../log/logger.h"

#define PY_SSIZE_T_CLEAN
#undef _DEBUG
#include <Python.h>

namespace PythonLoader
{
    template<typename T>
    concept POData = std::is_pod<T>::value && !std::is_pointer<T>::value;

    template<POData D>
    struct PythonReturn
    {
        PythonReturn() : Valid(false) {  }
        explicit PythonReturn(D data) : Valid(true), Data(data) {  }

        bool Valid;
        D Data;
    };

    struct PythonParam
    {
        template<typename T>
        PythonParam(T t) : type(std::type_index(typeid(T)))
        {
            if constexpr(std::is_pod<T>::value)
            {
                data_size = sizeof(T);
                data = (void*)new char[data_size];
                memcpy(data, &t, data_size);
            }
            else
            {
                L_ERROR("PythonParam supports only POD types.");
            }
        }
        template<>
        PythonParam(std::string t) : type(std::type_index(typeid(std::string)))
        {
            data_size = sizeof(std::string);
            data = (void*)new char[data_size];
            std::string* s = new (data) std::string(t);
        }

        ~PythonParam(){ if(data) delete[] data; }

        PythonParam(const PythonParam& pp) : type(pp.type)
        {
            data = new char[pp.data_size];
            memcpy(data, pp.data, pp.data_size);
            data_size = pp.data_size;
        }

        size_t data_size = 0;
        void* data = nullptr;
        std::type_index type;
    };

    template<typename T>
    std::deque<PythonParam> GetParams(T t)
    {
        std::deque<PythonParam> types;
        types.push_front(PythonParam(t));
        return types;
    }

    template<typename T, typename... Args>
    std::deque<PythonParam> GetParams(T t, Args... args)
    {
        std::deque<PythonParam> types = GetParams(args...);
        types.push_front(PythonParam(t));
        return types;
    }

    template<POData RT, typename... Args>
    PythonReturn<RT> RunPythonScriptMain(const std::string& script_name, Args... args)
    {
        PyObject* pName;
        PyObject* pModule;
        PyObject* pFunc;
        PyObject* pArgs;
        PyObject* pValue;

        Py_Initialize();
        pName = PyUnicode_DecodeFSDefault(script_name.c_str());
        if(!pName)
        {
            L_ERROR("PyUnicode_DecodeFSDefault failed: %s", script_name.c_str());
            return PythonReturn<RT>();
        }
        
        std::filesystem::path scripts_folder(std::filesystem::current_path() / "scripts");

        PyObject* sysPath = PySys_GetObject((char*)"path");
        PyObject* programName = PyUnicode_FromString(scripts_folder.string().c_str());
        PyList_Append(sysPath, programName);
        Py_DECREF(programName);
        pModule = PyImport_Import(pName);
        Py_DECREF(pName);
        if(!pModule)
        {
            if (PyErr_Occurred())
                PyErr_Print();
            L_ERROR("%s is not a valid module.", script_name.c_str());
            return PythonReturn<RT>();
        }

        pFunc = PyObject_GetAttrString(pModule, "main");

        if(!pFunc)
        {
            if (PyErr_Occurred())
                PyErr_Print();
            L_ERROR("Could not find function 'main'.");
            L_ERROR("A valid python script must contain a 'main' function.");
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            return PythonReturn<RT>();
        }

        if(!PyCallable_Check(pFunc))
        {
            L_ERROR("Found variable 'main', but it is not callable.");
            L_ERROR("A valid python script must contain a 'main' function.");
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
            return PythonReturn<RT>();
        }

        pArgs = PyTuple_New(sizeof...(Args));
        std::deque<PythonParam> pparams = GetParams(args...);
        

        // I know: C-style casts...
        int i = 0;
        while(!pparams.empty())
        {
            PythonParam& p = pparams.front();

            if(std::type_index(typeid(int)) == p.type || std::type_index(typeid(long)) == p.type)
            {
                pValue = PyLong_FromLong(*(long*)p.data);
            }
            else if(std::type_index(typeid(double)) == p.type)
            {
                pValue = PyFloat_FromDouble(*(double*)p.data);
            }
            else if(std::type_index(typeid(std::string)) == p.type)
            {
                pValue = PyBytes_FromString(((std::string*)p.data)->c_str());
            }
            else
            {
                L_ERROR("Type %s is not supported as a 'PythonParam'.", p.type.name());
                Py_DECREF(pArgs);
                Py_DECREF(pModule);
                return PythonReturn<RT>();
            }

            if(!pValue)
            {
                Py_DECREF(pArgs);
                Py_DECREF(pModule);
                L_ERROR("Cannot convert argument.");
                return PythonReturn<RT>();
            }

            PyTuple_SetItem(pArgs, i++, pValue);
            pparams.pop_front();
        }

        pValue = PyObject_CallObject(pFunc, pArgs);
        Py_DECREF(pArgs);
        if (!pValue)
        {
            Py_DECREF(pFunc);
            Py_DECREF(pModule);
            PyErr_Print();
            L_ERROR("Call failed.");
            return PythonReturn<RT>();
        }

        RT lstruct = *((RT*)PyBytes_AsString(pValue));
        Py_DECREF(pValue);
        PythonReturn<RT> ret(lstruct);

        if (Py_FinalizeEx() < 0)
        {
            L_ERROR("Py_FinalizeEx() failed.");
            return PythonReturn<RT>();
        }

        return ret;
    }
}
