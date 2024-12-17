#include "pydt.h"


/**
 * Module infomation
 */
#define MODULE_NAME "_fdt"
#define MODULE_VERSION "0.0.1"
static PyTypeObject FDT_type;




/** 
 * Set the corresponding Python exception and return -1
 */
int _set_py_except(int errval, int *fdt_errno)
{
    if (fdt_errno != NULL) {
        *fdt_errno = errval;
    }

    switch (errval) {
    case -FDT_ERR_NOTFOUND:
        PyErr_SetString(PyExc_ValueError, "node or property not found");
        break;
    case -FDT_ERR_EXISTS:
        PyErr_SetString(PyExc_ValueError, "node or property already exists");
        break;
    case -FDT_ERR_NOSPACE:
        PyErr_SetString(PyExc_ValueError, "not enough space to expand");
        break;
    case -FDT_ERR_BADOFFSET:
        PyErr_SetString(PyExc_ValueError, "invalid offset");
        break;
    case -FDT_ERR_BADPATH:
        PyErr_SetString(PyExc_ValueError, "bad path format");
        break;
    case -FDT_ERR_BADPHANDLE:
        PyErr_SetString(PyExc_ValueError, "invalid phandle");
        break;
    case -FDT_ERR_BADSTATE:
        PyErr_SetString(PyExc_ValueError, "incomplete devicetree");
        break;
    case -FDT_ERR_TRUNCATED:
        PyErr_SetString(PyExc_ValueError, "devicetree is truncated");
        break;
    case -FDT_ERR_BADMAGIC:
        PyErr_SetString(PyExc_ValueError, "invalid magic number");
        break;
    case -FDT_ERR_BADVERSION:
        PyErr_SetString(PyExc_ValueError, "unsupported devicetree version");
        break;
    case -FDT_ERR_BADSTRUCTURE:
        PyErr_SetString(PyExc_ValueError, "corrupt devicetree structure");
        break;
    case -FDT_ERR_BADLAYOUT:
        PyErr_SetString(PyExc_ValueError, "incorrect devicetree layout");
        break;
    case -FDT_ERR_INTERNAL:
        PyErr_SetString(PyExc_RuntimeError, "internal error (bug in libfdt)");
        break;
    case -FDT_ERR_BADNCELLS:
        PyErr_SetString(PyExc_ValueError, "invalid #xxx-cells");
        break;
    case -FDT_ERR_BADVALUE:
        PyErr_SetString(PyExc_ValueError, "unexpected property value");
        break;
    case -FDT_ERR_BADOVERLAY:
        PyErr_SetString(PyExc_ValueError, "invalid devicetree overlay");
        break;
    case -FDT_ERR_NOPHANDLES:
        PyErr_SetString(PyExc_ValueError, "no phandles available");
        break;
    case -FDT_ERR_BADFLAGS:
        PyErr_SetString(PyExc_ValueError, "invalid flags");
        break;
    case -FDT_ERR_ALIGNMENT:
        PyErr_SetString(PyExc_ValueError, "misaligned devicetree");
        break;
    default:
        PyErr_SetString(PyExc_RuntimeError, "unknown error");
        break;
    }

    return -1;
}

/**
 * Get the offset of the specified path, setting a Python exception
 * and return -1 if not found.
 */
int _get_fdt_path_offset(const void *fdt, const char *path, int *fdt_errno)
{
    int offset;

    offset = fdt_path_offset(fdt, path);
    if (offset < 0) {
        return _set_py_except(offset, fdt_errno);
    }

    return offset;
}

/** 
 * Traverse all properties under a specified path based on the given
 * offset and return a Python dict containing the properties and their values.
 */
static PyObject *_foreach_node_props_by_offset(const void *fdt, int offset)
{
    uint32_t ret;
    int props_offset, len;
    struct fdt_property *prop;
    const char *propname;
    PyObject *value;
    PyObject *props = PyDict_New();

    if (props == NULL) {
        PyErr_SetString(PyExc_RuntimeError,
                        "dict 'props' instantiation failed");
        return NULL;
    }

    if (offset == -1) {
        return NULL;
    }

    /**  */
    fdt_for_each_property_offset(props_offset, fdt, offset) {
        prop = fdt_get_property_by_offset(fdt, props_offset, &len);
        propname = fdt_string(fdt, fdt32_to_cpu(prop->nameoff));

        if (len < 0 || prop->data == NULL) {
            value = Py_None;
            goto next;
        }

        /* compatible attribute */
        if (!strcmp(propname, "compatible")) {
            value = PyUnicode_FromStringAndSize(prop->data, len - 1);
            goto next;
        }
        if (len % 4 == 0) {
            value = PyList_New(len / 4);
            /* boolean attribute */
            if (len == 0) {
                PyList_Append(value, Py_True);
                goto next;
            }
            /* multi-value attribute */
            for (int i = 0; i < len / 4; i++) {
                ret = fdt32_to_cpu(((fdt32_t *)prop->data)[i]);
                PyList_SetItem(value, i, PyUnicode_FromFormat("0x%x", ret));
            }
        }

    next:
        PyDict_SetItemString(props, propname, value);
    }

    Py_DECREF(value);
    return props;
}

const void *_read_dtb_file(const char *filename)
{
    struct stat s;
    char *fdt;
    FILE *fp;

    if ((fp = fopen(filename, "rb")) == NULL) {
        return NULL;
    }

    if (stat(filename, &s)) {
        return NULL;
    }

    fdt = PyMem_Malloc(s.st_size);
    fread(fdt, s.st_size, sizeof(char), fp);
    fclose(fp);
    return fdt;

error:
    fclose(fp);
    return NULL;
}

bool _is_dtb_file(const char *filename)
{
    size_t len = strlen(filename);
    const char *prefix = filename + len - 3;

    return !strcmp(prefix, "dtb") ? true : false;
}

/**
 * FDT objects methods
 */
static PyObject *get_props_by_path(FDTObject *self, PyObject *args)
{
    int offset;
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    offset = _get_fdt_path_offset(self->fdt, path, &(self->fdt_errno));
    if (offset == -1) {
        return NULL;
    }

    return _foreach_node_props_by_offset(self->fdt, offset);
}

static PyObject *get_node_offset_by_path(FDTObject *self, PyObject *args)
{
    int offset;
    const char *path;

    if (!PyArg_ParseTuple(args, "s", &path)) {
        return NULL;
    }

    offset = _get_fdt_path_offset(self->fdt, path, &(self->fdt_errno));
    if (offset == -1) {
        return NULL;
    }

    return PyLong_FromLong(offset);
}

static PyObject *get_node_path_by_alias(FDTObject *self, PyObject *args)
{
    const char *name;
    const char *alias;

    if (!PyArg_ParseTuple(args, "s", &name)) {
        return NULL;
    }

    alias = fdt_get_alias(self->fdt, name);
    if (alias == NULL) {
        return Py_None;
    }

    return PyUnicode_FromString(alias);
}

static PyObject *get_node_name_by_offset(FDTObject *self, PyObject *args)
{
    int offset, ret;
    const char *name;

    if (!PyArg_ParseTuple(args, "i", &offset)) {
        return NULL;
    }

    name = fdt_get_name(self->fdt, offset, &ret);
    if (name == NULL) {
        _set_py_except(ret, &(self->fdt_errno));
        return NULL;
    }

    return PyUnicode_FromString(name);
}

static PyObject *get_node_offset_by_compat(FDTObject *self, PyObject *args)
{
    int offset;
    const char *compatible;

    if (!PyArg_ParseTuple(args, "s", &compatible)) {
        return NULL;
    }

    /* start the search from the root node. */
    offset = fdt_node_offset_by_compatible(self->fdt, -1, compatible);
    if (offset < 0) {
        _set_py_except(offset, &(self->fdt_errno));
        return NULL;
    }

    return PyLong_FromLong(offset);
}

static PyObject *get_node_path_by_offset(FDTObject *self, PyObject *args)
{
    int offset, ret;
    char *name = PyMem_Malloc(FDT_FULL_PATH_LEN);

    if (name == NULL) {
        PyErr_NoMemory();
        return NULL;
    }

    if (!PyArg_ParseTuple(args, "i", &offset)) {
        return NULL;
    }

    ret = fdt_get_path(self->fdt, offset, name, FDT_FULL_PATH_LEN);
    if (ret < 0) {
        _set_py_except(ret, &(self->fdt_errno));
        return NULL;
    }

    return PyUnicode_FromString(name);
}

static PyObject *get_props_by_offset(FDTObject *self, PyObject *args)
{
    int offset;
    PyObject *props;

    if (!PyArg_ParseTuple(args, "i", &offset)) {

    }

    props = _foreach_node_props_by_offset(self->fdt, offset);
    if (props == NULL) {
        return Py_None;
    }

    return props;
}

static PyObject *get_phandle_by_offset(FDTObject *self, PyObject *args)
{
    uint32_t phandle;
    int offset;

    if (!PyArg_ParseTuple(args, "i", &offset)) {
        return NULL;
    }

    phandle = fdt_get_phandle(self->fdt, offset);
    return phandle > 0 ? PyUnicode_FromFormat("0x%x", phandle) : Py_None;
}

static PyObject *get_props_by_compat(FDTObject *self, PyObject *args)
{
    int offset;
    const char *compatible;

    if (!PyArg_ParseTuple(args, "s", &compatible)) {
        return NULL;
    }

    /* start the search from the root node. */
    offset = fdt_node_offset_by_compatible(self->fdt, -1, compatible);
    if (offset < 0) {
        _set_py_except(offset, &(self->fdt_errno));
        return NULL;
    }

    return _foreach_node_props_by_offset(self->fdt, offset);
}

static PyMethodDef fdt_obj_methods[] = {
    /**
     * get node property
     */
    {"get_props_by_path", (PyCFunction)get_props_by_path, METH_VARARGS,
    PyDoc_STR(
    "get_props_by_path($self, path)\n--\n"
    "Retrieves all properties of a node at a given path in "
    "the FDT and returns them as a dictionary.\n"
    "If the path does not exist, it raises a ValueError."
    )},
    {"get_props_by_offset", (PyCFunction)get_props_by_offset, METH_VARARGS,
    PyDoc_STR(
    "get_props_by_offset($self, offset)\n--\n"
    "Retrieves all properties of a node at a given offset in "
    "the FDT and returns them as a dictionary.\n"
    "If the offset is invalid, it raises a ValueError."
    )},
    {"get_props_by_compat", (PyCFunction)get_props_by_compat, METH_VARARGS,
    PyDoc_STR(
    "get_props_by_offset($self, offset)\n--\n"
    "Retrieves all properties of a node at a given compatible in "
    "the FDT and returns them as a dictionary.\n"
    "If the compatible is not found, it raises a ValueError."
    )},
    /**
     * node offset
     */
    {"get_node_offset_by_path", (PyCFunction)get_node_offset_by_path,
    METH_VARARGS,
    PyDoc_STR(
    "get_node_offset_by_path($self, path)\n--\n"
    "Retrieves the offset of a node in the FDT for a given path.\n"
    "If the path does not exist, it raises a ValueError."
    )},
    {"get_node_offset_by_compat", (PyCFunction)get_node_offset_by_compat,
    METH_VARARGS,
    PyDoc_STR(
    "get_node_offset_by_compat($self, compatible)\n--\n"
    "Searches for the given compatible and returns the offset of the "
    "node containing it in the FDT.\n"
    "If the compatible is not found, it raises a ValueError."
    )},
    /**
     * node path
     */
    {"get_node_path_by_alias", (PyCFunction)get_node_path_by_alias,
    METH_VARARGS,
    PyDoc_STR(
    "get_node_path_by_alias($self, name)\n--\n"
    "Retrieves the full node path associated with the given alias in the FDT.\n"
    "If the alias does not exist, it returns None."
    )},
    {"get_node_path_by_offset", (PyCFunction)get_node_path_by_offset,
    METH_VARARGS,
    PyDoc_STR(
    "get_node_path_by_offset($self, offset)\n--\n"
    "Retrieves the full node path associated with the given offset in the FDT.\n"
    "If the offset is invalid, it raises a ValueError."
    )},
    /**
     * node name
     */
    {"get_node_name_by_offset", (PyCFunction)get_node_name_by_offset,
    METH_VARARGS,
    PyDoc_STR(
    "get_node_name_by_offset($self, offset)\n--\n"
    "Retrieves the name of the node at a given offset in the FDT.\n"
    "If the offset is invalid, it raises a ValueError."
    )},
    /**
     * phandle
     */
    {"get_phandle_by_offset", (PyCFunction)get_phandle_by_offset, METH_VARARGS,
    PyDoc_STR(
    "get_phandle_by_offset($self, offset)\n--\n"
    "Retrieves the phandle value of the node at the given offset in the FDT.\n"
    "If the offset is invalid, it returns None."
    )},
    {NULL},
};

static PyObject *fdt_obj_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    const char *filename;
    FDTObject *fdtobject;
    char *kwlist[] = {"filename", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s",
                                     kwlist, &filename)) {
        return NULL;
    }

    fdtobject = PyObject_New(FDTObject, &FDT_type);

    if (!_is_dtb_file(filename)) {
        PyErr_Format(PyExc_RuntimeError, "'%s' is not a valid DTB file",
                     filename);
        return NULL;
    }

    fdtobject->fdt = _read_dtb_file(filename);
    if (fdtobject->fdt == NULL) {
        PyErr_Format(PyExc_OSError, "failed to open file '%s'", filename);
        return NULL;
    }

    if (fdt_check_header(fdtobject->fdt)) {
        PyErr_Format(PyExc_ValueError, "'%s' is not a valid FDT file",
                     filename);
        return NULL;
    }
    fdtobject->fdt_errno = FDT_ERR_MAX;
    fdtobject->magic = fdt_magic(fdtobject->fdt);
    fdtobject->version = fdt_version(fdtobject->fdt);
    fdtobject->totalsize = fdt_totalsize(fdtobject->fdt);
    fdtobject->headersize = fdt_header_size(fdtobject->fdt);

    return (PyObject *)fdtobject;
}

static void fdt_obj_free(void *args)
{
}

static PyMemberDef fdt_obj_members[] = {
    {"magic", Py_T_UINT, offsetof(FDTObject, magic), Py_READONLY,
    PyDoc_STR("FDT magic number")},
    {"version", Py_T_UINT, offsetof(FDTObject, version), Py_READONLY,
    PyDoc_STR("FDT version")},
    {"totalsize", Py_T_UINT, offsetof(FDTObject, totalsize), Py_READONLY,
    PyDoc_STR("FDT totalsize")},
    {"errno", Py_T_INT, offsetof(FDTObject, fdt_errno), Py_READONLY,
    PyDoc_STR("FDT error code")},
    {"headersize", Py_T_ULONGLONG, offsetof(FDTObject, headersize), Py_READONLY,
    PyDoc_STR("FDT header size")},
    {NULL},
};

PyDoc_STRVAR(fdt_obj_doc,
"_fdt.FDT(filename)\n--\n"
"Read the dtb in the given filename and create a corresponding FDT object. \n"
"It is allowed to call high-level encapsulated fdt functions on this object, "
"or use low-level API.\n"
""
);
static PyTypeObject FDT_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "_fdt.FDT",
    .tp_basicsize = sizeof(FDTObject),
    .tp_new = fdt_obj_new,
    .tp_free = fdt_obj_free,
    .tp_methods = fdt_obj_methods,
    .tp_members = fdt_obj_members,
    .tp_doc = fdt_obj_doc,
};

/**
 * module methods
 */
static PyMethodDef fdt_methods[] = {
    {NULL},
};

/**
 * module slots
 */
static int pydt_exec(PyObject *m)
{
#define ADD_ERR_MACRO(mod, name)                                  \
    do {                                                      \
        if (PyModule_AddIntConstant(mod, #name, -name) < 0) { \
            return -1;                                        \
        }                                                     \
    } while(0)

    ADD_ERR_MACRO(m, FDT_ERR_NOTFOUND);
    ADD_ERR_MACRO(m, FDT_ERR_EXISTS);
    ADD_ERR_MACRO(m, FDT_ERR_NOSPACE);
    ADD_ERR_MACRO(m, FDT_ERR_BADOFFSET);
    ADD_ERR_MACRO(m, FDT_ERR_BADPATH);
    ADD_ERR_MACRO(m, FDT_ERR_BADPHANDLE);
    ADD_ERR_MACRO(m, FDT_ERR_BADSTATE);
    ADD_ERR_MACRO(m, FDT_ERR_TRUNCATED);
    ADD_ERR_MACRO(m, FDT_ERR_BADMAGIC);
    ADD_ERR_MACRO(m, FDT_ERR_BADVERSION);
    ADD_ERR_MACRO(m, FDT_ERR_BADSTRUCTURE);
    ADD_ERR_MACRO(m, FDT_ERR_BADLAYOUT);
    ADD_ERR_MACRO(m, FDT_ERR_INTERNAL);
    ADD_ERR_MACRO(m, FDT_ERR_BADNCELLS);
    ADD_ERR_MACRO(m, FDT_ERR_BADVALUE);
    ADD_ERR_MACRO(m, FDT_ERR_BADOVERLAY);
    ADD_ERR_MACRO(m, FDT_ERR_NOPHANDLES);
    ADD_ERR_MACRO(m, FDT_ERR_BADFLAGS);

#undef ADD_ERR_MACRO

    if (PyModule_AddStringConstant(m, "__version__", MODULE_VERSION) < 0) {
        return -1;
    }

    if (PyType_Ready(&FDT_type) < 0) {
        return -1;
    }
    if (PyModule_AddObjectRef(m, "FDT", (PyObject *)&FDT_type) < 0) {
        return -1;
    }

    return 0;
}

static PyModuleDef_Slot fdt_slots[] = {
    {Py_mod_exec, pydt_exec},
    {0, NULL},
};

/**
 * module
 */
PyDoc_STRVAR(fdt_doc,
"Wraps low-level libfdt library functions."
);
static PyModuleDef fdt_module = {
    .m_name = MODULE_NAME,
    .m_doc = fdt_doc,
    .m_methods = fdt_methods,
    .m_slots = fdt_slots,
};

PyMODINIT_FUNC PyInit__fdt(void)
{
    return PyModuleDef_Init(&fdt_module);
}
