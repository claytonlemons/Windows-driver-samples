#pragma once

#include <windows.h>

#define ERROR_IS_FATAL(error) (*error != NOERROR)
#define ERROR_MATCHES(error, code) (*error == code)
#define GET_LAST_ERROR(error) (*error = GetLastError())
#define CLEAR_ERROR(error) (*error = NOERROR)
#define PRINT_ERROR(error, message) (printf(message, *error))

typedef DWORD ERR;

