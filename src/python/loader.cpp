#include "loader.h"

PythonLoader::PyEnvExt PythonLoader::StartPythonScriptEnv(const std::string& script_name)
{
    PyEnvExt ret;
    Py_Initialize();
    PyObject* pName = PyUnicode_DecodeFSDefault(script_name.c_str());
    if(!pName)
    {
        L_ERROR("PyUnicode_DecodeFSDefault failed: %s", script_name.c_str());
        return ret;
    }
    
    std::filesystem::path scripts_folder(std::filesystem::current_path() / "scripts");

    PyObject* sysPath = PySys_GetObject((char*)"path");
    PyObject* programName = PyUnicode_FromString(scripts_folder.string().c_str());
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
    }
    else
    {
        L_ERROR("Cannot close an invalid PyEnvExt.");
    }

    if (Py_FinalizeEx() < 0)
    {
        L_ERROR("Py_FinalizeEx() failed.");
    }
}
