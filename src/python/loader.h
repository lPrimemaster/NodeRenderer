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
    concept POData = std::is_pod<T>::value;

    template<typename T>
    concept ClassData = std::is_class<T>::value;

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
        template<typename T> requires POData<T> || ClassData<T>
        PythonParam(T t);

        template<POData T>
        PythonParam(T t) : type(std::type_index(typeid(T)))
        {
            data_size = sizeof(T);
            data = (void*)new char[data_size];
            memcpy(data, &t, data_size);
        }

        template<ClassData T>
        PythonParam(T t) : type(std::type_index(typeid(T)))
        {
            data_size = sizeof(T);
            data = (void*)new char[data_size];
            T* i = new (data) T(t);
        }

        PythonParam(const PythonParam& pp) : type(pp.type)
        {
            data = new char[pp.data_size];
            memcpy(data, pp.data, pp.data_size);
            data_size = pp.data_size;
        }

        ~PythonParam(){ if(data) delete[] data; }

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

    struct PyEnvExt
    {
        PyObject* pModule = nullptr;
        PyGILState_STATE gstate;
    };

    PyEnvExt StartPythonScriptEnv(const std::string& script_name);

    void EndPythonScriptEnv(PyEnvExt& env);

    void Init();
    void Deinit();

    template<POData RT, typename... Args>
    PythonReturn<RT> RunPythonScriptFunction(const PyEnvExt& env, const std::string& func_name, Args... args)
    {
        if(!env.pModule)
        {
            L_ERROR("PyEnvExt is invalid. Did you call StartPythonScriptEnv?");
            return PythonReturn<RT>();
        }

        PyObject* pFunc;
        PyObject* pArgs;
        PyObject* pValue;

        pFunc = PyObject_GetAttrString(env.pModule, func_name.c_str());

        if(!pFunc)
        {
            if (PyErr_Occurred())
                PyErr_Print();
            L_ERROR("Could not find function '%s'.", func_name.c_str());
            L_ERROR("A valid python script must contain the specified function.", func_name.c_str());
            Py_XDECREF(pFunc);
            return PythonReturn<RT>();
        }

        if(!PyCallable_Check(pFunc))
        {
            L_ERROR("Found variable '%s', but it is not callable.", func_name.c_str());
            L_ERROR("A valid python script must contain the specified function.");
            Py_XDECREF(pFunc);
            return PythonReturn<RT>();
        }

        if constexpr(sizeof...(Args) > 0)
        {
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
                    return PythonReturn<RT>();
                }

                if(!pValue)
                {
                    Py_DECREF(pArgs);
                    L_ERROR("Cannot convert argument.");
                    return PythonReturn<RT>();
                }

                PyTuple_SetItem(pArgs, i++, pValue);
                pparams.pop_front();
            }
        }
        else
        {
            pArgs = nullptr;
        }

        pValue = PyObject_CallObject(pFunc, pArgs);
        if(pArgs) Py_DECREF(pArgs);
        if (!pValue)
        {
            Py_DECREF(pFunc);
            PyErr_Print();
            L_ERROR("Call failed.");
            return PythonReturn<RT>();
        }

        RT lstruct = *((RT*)PyBytes_AsString(pValue));
        Py_DECREF(pValue);
        return PythonReturn<RT>(lstruct);
    }

    template<typename... Args>
    bool RunPythonScriptFunctionCopyReturn(const PyEnvExt& env, const std::string& func_name, char* return_buffer, size_t size, Args... args)
    {
        if(!env.pModule)
        {
            L_ERROR("PyEnvExt is invalid. Did you call StartPythonScriptEnv?");
            return false;
        }

        PyObject* pFunc;
        PyObject* pArgs;
        PyObject* pValue;

        pFunc = PyObject_GetAttrString(env.pModule, func_name.c_str());

        if(!pFunc)
        {
            if (PyErr_Occurred())
                PyErr_Print();
            L_ERROR("Could not find function '%s'.", func_name.c_str());
            L_ERROR("A valid python script must contain the specified function.", func_name.c_str());
            Py_XDECREF(pFunc);
            return false;
        }

        if(!PyCallable_Check(pFunc))
        {
            L_ERROR("Found variable '%s', but it is not callable.", func_name.c_str());
            L_ERROR("A valid python script must contain the specified function.");
            Py_XDECREF(pFunc);
            return false;
        }

        if constexpr(sizeof...(Args) > 0)
        {
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
                    return false;
                }

                if(!pValue)
                {
                    Py_DECREF(pArgs);
                    L_ERROR("Cannot convert argument.");
                    return false;
                }

                PyTuple_SetItem(pArgs, i++, pValue);
                pparams.pop_front();
            }
        }
        else
        {
            pArgs = nullptr;
        }

        pValue = PyObject_CallObject(pFunc, pArgs);
        if(pArgs) Py_DECREF(pArgs);
        if (!pValue)
        {
            Py_DECREF(pFunc);
            PyErr_Print();
            L_ERROR("Call failed.");
            return false;
        }

        char* src = PyBytes_AsString(pValue);
        memcpy(return_buffer, src, size);
        Py_DECREF(pValue);
        return true;
    }
}
