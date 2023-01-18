#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <errno.h>
#include <dlfcn.h>
#endif
#include <string>
#include <map>
using namespace std;

class DLLMap : map<string, void*> {
public:
    static DLLMap& getInstance() {
        static DLLMap ins;
        return ins;
    }

private:
    DLLMap() {}
    DLLMap(DLLMap const&);	  // Don't implement to guarantee singleton semantics
    void operator=(DLLMap const&); // Don't implement to guarantee singleton semantics

public:
    void* getProcAddress(string dll, string functionName)
    {
        if (find(dll) == end()) {
#ifdef _WIN32
            void* mod = LoadLibraryA(dll.c_str());
            if (mod == NULL) {
                throw system_error(GetLastError(), system_category(), "LoadLibraryA failed");
            }
#else
            void* mod = dlopen(dll.c_str(), RTLD_LAZY);
            if (mod == NULL) {
                throw system_error(errno, system_category(), dlerror(););
            }
#endif 
            (*this)[dll] = mod;
        }

        void* mod = (*this)[dll];
#ifdef _WIN32
        void* fp = GetProcAddress((HMODULE)mod, functionName.c_str());
        if (fp == nullptr) {
            throw system_error(GetLastError(), system_category(), "LoadLibraryA failed");
        }
#else
        void* fp = dlsym(mod, functionName.c_str());
        if (fp == NULL) {
            throw system_error(errno, system_category(), dlerror(););
        }
#endif   
        return fp;
    }
    virtual ~DLLMap()
    {
        for (auto modPair : (*this)){
#ifdef _WIN32
            FreeLibrary((HMODULE)modPair.second);
#else
            dlclose(modPair.second);
#endif
        }
    }
};


template <typename R, typename... T>
class dllfunctor_stdcall {
public:
    dllfunctor_stdcall(string dll, string function)
    {
        _f = (R(__stdcall *)(T...))DLLMap::getInstance().getProcAddress(dll, function.c_str());
    }
    R operator()(T... args) { return _f(args...); }

private:
    R(__stdcall *_f)(T...);
};