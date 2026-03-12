from setuptools import setup
from pathlib import Path
from os import system
system(f"python {Path(__file__).parent / 'collect.py'} python")

setup(
    name="libchess",
    version="1.0.0",
    description="Chess library with C++ bindings",
    packages=["libchess"],
    package_data={
        "libchess": [
            "*.dll",
            "*.pyd",
            "*.pyi",
            "*.py"
            "py.typed",
        ],
    },
    include_package_data=True,
    python_requires=">=3.6",
)