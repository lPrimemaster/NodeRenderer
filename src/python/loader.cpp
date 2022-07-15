#include "loader.h"

PythonLoader::PyEnvExt PythonLoader::StartPythonScriptEnv(const std::string& script_name)
{

    PyEnvExt ret;
    ret.gstate = PyGILState_Ensure();

    // NOTE: Script name may not be wchar
    PyObject* pName = PyUnicode_DecodeFSDefault(script_name.c_str());
    if(!pName)
    {
        L_ERROR("PyUnicode_DecodeFSDefault failed: %s", script_name.c_str());
        return ret;
    }
    
    std::filesystem::path scripts_folder(std::filesystem::current_path() / "scripts");

    PyObject* sysPath = PySys_GetObject((char*)"path");
    PyObject* programName = PyUnicode_FromWideChar(scripts_folder.wstring().c_str(), -1);
    PyList_Append(sysPath, programName);
    Py_DECREF(programName);
    PyObject* pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    if(!pModule)
    {
        if (PyErr_Occurred())
            PyErr_Print();
        L_ERROR("%s is not a valid module.", script_name.c_str());
        return ret;
    }

    L_DEBUG("Loaded python module: %s", script_name.c_str());
    ret.pModule = pModule;
    return ret;
}

void PythonLoader::EndPythonScriptEnv(PythonLoader::PyEnvExt& env)
{
    if(env.pModule != nullptr)
    {
        Py_DECREF(env.pModule);
        env.pModule = nullptr;
        PyGILState_Release(env.gstate);
    }
    else
    {
        L_ERROR("Cannot close an invalid PyEnvExt.");
    }
}

void PythonLoader::Init()
{
    Py_Initialize();
    PyEval_InitThreads();
}

void PythonLoader::Deinit()
{
    if (Py_FinalizeEx() < 0)
    {
        L_ERROR("Py_FinalizeEx() failed.");
    }
}
