#ifndef PYFDT_FDTMODULE_H_
#define PYFDT_FDTMODULE_H_

/* include Python.h */
#define PY_SSIZE_T_CLEAN
#include "pythoncapi_compat.h"

/* include POSIX */
#include <fcntl.h>
#include <sys/stat.h>

/* include standard */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

/* */
#include <libfdt.h>

#ifndef FDT_FULL_PATH_LEN
# define FDT_FULL_PATH_LEN 0x100 /* 256 */
#endif

typedef struct {
    PyObject_HEAD
    int fdt_errno;

    /* Raw FDT */
    uint32_t magic;
    uint32_t version;
    uint32_t totalsize;
    const void *fdt;
}FDTObject;


#endif // PYFDT_FDTMODULE_H_