#ifndef COMPATIBILITY_H_
#define COMPATIBILITY_H_

#ifdef _WIN32
  #define DLLExport __declspec(dllexport)
  #define DLLImport __declspec(dllimport)
  #define EXTERN_IMPORT extern
#else
  #define DLLExport
  #define DLLImport
  #define EXTERN_IMPORT
#endif

#endif /*COMPATIBILITY_H_*/