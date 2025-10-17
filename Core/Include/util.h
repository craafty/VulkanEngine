#pragma once
#include <string>
#include <assert.h>
#include <cstdarg>
#include <cstdio>

#define NOMINMAX 
#ifdef _WIN32
	#include <windows.h>
#endif

bool ReadFile(const char* fileName, std::string& outFile);
char* ReadBinaryFile(const char* pFileName, int& size);

void WriteBinaryFile(const char* pFilename, const void* pData, int size);

void myError(const char* pFileName, unsigned int line, const char* format, ...);
void myFileError(const char* pFileName, unsigned int line, const char* pFileError);

#define MY_ERROR(...) myError(__FILE__, __LINE__, __VA_ARGS__)
#define MY_FILE_ERROR(FileError) myFileError(__FILE__, __LINE__, FileError);

#define ARRAY_SIZE_IN_ELEMENTS(a) (sizeof(a)/sizeof(a[0]))
#define ARRAY_SIZE_IN_BYTES(a) (sizeof(a[0]) * a.size())

std::string GetDirFromFilename(const std::string& Filename);

#define CLAMP(Val, Start, End) std::min(std::max((Val), (Start)), (End));