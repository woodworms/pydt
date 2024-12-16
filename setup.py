from setuptools import setup, find_packages, Extension
from glob import glob
import os


# libfdt library source files
libfdts = [os.path.join(i) for i in glob('libfdt/*.c')]


setup(
    name="pydt",
    version='0.0.1',
    description="More powerful Python-bindings for libfdt",
    keywords="devicetree, dts, dtb, generator",
    license="GPL",
    author="James Roy",
    packages=find_packages(),
    ext_modules=[
        Extension("_fdt", [f"pydt/_pydt/fdtmodule.c"] + libfdts, ["libfdt/"]),
    ],
    classifiers=[
        "License :: OSI Approved :: GPL License",
        "Natural Language :: English",
        "Operating System :: POSIX",
        "Programming Language :: C",
        "Programming Language :: Python :: 3 :: Only",
        "Programming Language :: Python :: Implementation :: CPython",
        "Programming Language :: Python :: Implementation :: PyPy",
        "Topic :: Software Development :: Libraries :: Python Modules"
    ],
)