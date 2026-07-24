#include "atomic_file.h"

#include <errno.h>
#include <stdint.h>

#ifdef _WIN32
#include <io.h>
#include <wchar.h>
#include <windows.h>
#endif

static char *temporary_path_buffer(const char *path)
{
    // ".tmp." + process id + attempt + terminator, with ample decimal room
    // on every supported platform.
    static const size_t suffixCapacity = 64U;
    const size_t length = strlen(path);
    if (length > SIZE_MAX - suffixCapacity)
    {
        errno = ENAMETOOLONG;
        return NULL;
    }
    return (char *)malloc(length + suffixCapacity);
}

#ifdef _WIN32
static wchar_t *utf8_to_wide(const char *text)
{
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                              text, -1, NULL, 0);
    if (required <= 0) return NULL;
    wchar_t *wide = (wchar_t *)malloc((size_t)required * sizeof(wchar_t));
    if (!wide) return NULL;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            text, -1, wide, required) == 0)
    {
        free(wide);
        return NULL;
    }
    return wide;
}

static bool replace_file(const char *temporaryPath, const char *path)
{
    wchar_t *wideTemporary = utf8_to_wide(temporaryPath);
    wchar_t *widePath = utf8_to_wide(path);
    if (!wideTemporary || !widePath)
    {
        free(wideTemporary);
        free(widePath);
        return false;
    }
    const BOOL replaced = MoveFileExW(wideTemporary, widePath,
                                      MOVEFILE_REPLACE_EXISTING |
                                          MOVEFILE_WRITE_THROUGH);
    free(wideTemporary);
    free(widePath);
    return replaced != FALSE;
}

static void remove_utf8_file(const char *path)
{
    wchar_t *widePath = utf8_to_wide(path);
    if (widePath)
    {
        (void)DeleteFileW(widePath);
        free(widePath);
    }
}
#else
static bool replace_file(const char *temporaryPath, const char *path)
{
    return rename(temporaryPath, path) == 0;
}

static void remove_utf8_file(const char *path)
{
    (void)remove(path);
}
#endif

static FILE *create_exclusive_temporary_file(const char *path, char **temporaryPathOut)
{
    enum { MAX_TEMP_ATTEMPTS = 128 };
    char *temporaryPath = temporary_path_buffer(path);
    if (!temporaryPath) return NULL;
    const size_t capacity = strlen(path) + 64U;

    for (unsigned int attempt = 0U; attempt < MAX_TEMP_ATTEMPTS; attempt++)
    {
#ifdef _WIN32
        (void)snprintf(temporaryPath, capacity, "%s.tmp.%lu.%u", path,
                       (unsigned long)GetCurrentProcessId(), attempt);
        wchar_t *wideTemporary = utf8_to_wide(temporaryPath);
        if (!wideTemporary)
        {
            free(temporaryPath);
            return NULL;
        }
        HANDLE handle = CreateFileW(wideTemporary, GENERIC_WRITE, 0, NULL,
                                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
        const DWORD createError = GetLastError();
        free(wideTemporary);
        if (handle == INVALID_HANDLE_VALUE)
        {
            if (createError == ERROR_FILE_EXISTS || createError == ERROR_ALREADY_EXISTS)
            {
                continue;
            }
            free(temporaryPath);
            return NULL;
        }
        const int descriptor = _open_osfhandle((intptr_t)handle, _O_BINARY | _O_WRONLY);
        if (descriptor == -1)
        {
            (void)CloseHandle(handle);
            remove_utf8_file(temporaryPath);
            free(temporaryPath);
            return NULL;
        }
        FILE *file = _fdopen(descriptor, "wb");
        if (!file)
        {
            (void)_close(descriptor);
            remove_utf8_file(temporaryPath);
            free(temporaryPath);
            return NULL;
        }
#else
        (void)snprintf(temporaryPath, capacity, "%s.tmp.%ld.%u", path,
                       (long)getpid(), attempt);
        const int descriptor = open(temporaryPath, O_WRONLY | O_CREAT | O_EXCL, 0600);
        if (descriptor == -1)
        {
            if (errno == EEXIST) continue;
            free(temporaryPath);
            return NULL;
        }
        FILE *file = fdopen(descriptor, "wb");
        if (!file)
        {
            const int savedError = errno;
            (void)close(descriptor);
            remove_utf8_file(temporaryPath);
            free(temporaryPath);
            errno = savedError;
            return NULL;
        }
#endif
        *temporaryPathOut = temporaryPath;
        return file;
    }

    free(temporaryPath);
    errno = EEXIST;
    return NULL;
}

bool atomic_write_text_file(const char *path, const char *contents, size_t length)
{
    if (!path || path[0] == '\0' || !contents) return false;
    char *temporaryPath = NULL;
    FILE *file = create_exclusive_temporary_file(path, &temporaryPath);
    if (!file)
    {
        return false;
    }

    bool ok = fwrite(contents, 1U, length, file) == length;
    if (ok) ok = fflush(file) == 0;
#ifdef _WIN32
    if (ok) ok = _commit(_fileno(file)) == 0;
#else
    if (ok) ok = fsync(fileno(file)) == 0;
#endif
    if (fclose(file) != 0) ok = false;
    if (ok) ok = replace_file(temporaryPath, path);
    if (!ok)
    {
        const int savedError = errno;
        remove_utf8_file(temporaryPath);
        errno = savedError;
    }
    free(temporaryPath);
    return ok;
}
