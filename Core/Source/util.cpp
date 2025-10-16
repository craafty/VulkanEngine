#include <fstream>

#include "util.h"

bool ReadFile(const char* pFileName, std::string& outFile)
{
    std::ifstream f(pFileName);

    bool ret = false;

    if (f.is_open()) {
        std::string line;
        while (std::getline(f, line)) {
            outFile.append(line);
            outFile.append("\n");
        }

        f.close();

        ret = true;
    }
    else {
        MY_FILE_ERROR(pFileName);
    }

    return ret;
}


#ifdef _WIN32
char* ReadBinaryFile(const char* pFilename, int& size)
{
    FILE* f = NULL;

    errno_t err = fopen_s(&f, pFilename, "rb");

    if (!f) {
        char buf[256] = { 0 };
        strerror_s(buf, sizeof(buf), err);
        MY_ERROR("Error opening '%s': %s\n", pFilename, buf);
        exit(0);
    }

    struct stat stat_buf;
    int error = stat(pFilename, &stat_buf);

    if (error) {
        char buf[256] = { 0 };
        strerror_s(buf, sizeof(buf), err);
        MY_ERROR("Error getting file stats: %s\n", buf);
        return NULL;
    }

    size = stat_buf.st_size;

    char* p = (char*)malloc(size);
    assert(p);

    size_t bytes_read = fread(p, 1, size, f);

    if (bytes_read != size) {
        char buf[256] = { 0 };
        strerror_s(buf, sizeof(buf), err);
        MY_ERROR("Read file error file: %s\n", buf);
        exit(0);
    }

    fclose(f);

    return p;
}

void WriteBinaryFile(const char* pFilename, const void* pData, int size)
{
    FILE* f = NULL;

    errno_t err = fopen_s(&f, pFilename, "wb");

    if (!f) {
        MY_ERROR("Error opening '%s'\n", pFilename);
        exit(0);
    }

    size_t bytes_written = fwrite(pData, 1, size, f);

    if (bytes_written != size) {
        MY_ERROR("Error write file\n");
        exit(0);
    }

    fclose(f);
}

#else
char* ReadBinaryFile(const char* pFilename, int& size)
{
    FILE* f = fopen(pFilename, "rb");

    if (!f) {
        OGLDEV_ERROR("Error opening '%s': %s\n", pFilename, strerror(errno));
        exit(0);
    }

    struct stat stat_buf;
    int error = stat(pFilename, &stat_buf);

    if (error) {
        OGLDEV_ERROR("Error getting file stats: %s\n", strerror(errno));
        return NULL;
    }

    size = stat_buf.st_size;

    char* p = (char*)malloc(size);
    assert(p);

    size_t bytes_read = fread(p, 1, size, f);

    if (bytes_read != size) {
        OGLDEV_ERROR("Read file error file: %s\n", strerror(errno));
        exit(0);
    }

    fclose(f);

    return p;
}


void WriteBinaryFile(const char* pFilename, const void* pData, int size)
{
    FILE* f = fopen(pFilename, "wb");

    if (!f) {
        OGLDEV_ERROR("Error opening '%s': %s\n", pFilename, strerror(errno));
        exit(0);
    }

    int bytes_written = fwrite(pData, 1, size, f);

    if (bytes_written != size) {
        OGLDEV_ERROR("Error write file: %s\n", strerror(errno));
        exit(0);
    }

    fclose(f);

    /*    int f = open(pFilename, O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);

    if (f == -1) {
        OGLDEV_ERROR("Error opening '%s': %s\n", pFilename, strerror(errno));
        exit(0);
    }

    int write_len = write(f, pData, size);
    printf("%d\n", write_len);
    if (write_len != size) {
        OGLDEV_ERROR("Error write file: %s\n", strerror(errno));
        exit(0);
    }

    close(f);*/
}


#endif

void myError(const char* pFileName, unsigned int line, const char* format, ...)
{
	char msg[1000];
	va_list args;
	va_start(args, format);
#ifdef _WIN32
	vsnprintf_s(msg, sizeof(msg), _TRUNCATE, format, args);
#else
	vsnprintf(msg, sizeof(msg), format, args);
#endif
	va_end(args);

#ifdef _WIN32
	char msg2[1000];
	_snprintf_s(msg2, sizeof(msg2), "%s:%d: %s", pFileName, line, msg);
	MessageBoxA(NULL, msg2, NULL, 0);
#else
	fprintf(stderr, "%s:%d - %s", pFileName, line, msg);
#endif
}


void myFileError(const char* pFileName, unsigned int line, const char* pFileError)
{
#ifdef _WIN32
    char msg[1000];
    _snprintf_s(msg, sizeof(msg), "%s:%d: unable to open file `%s`", pFileName, line, pFileError);
    MessageBoxA(NULL, msg, NULL, 0);
#else
    fprintf(stderr, "%s:%d: unable to open file `%s`\n", pFileName, line, pFileError);
#endif
}