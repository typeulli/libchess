from setuptools import setup
from pathlib import Path
path_here = Path(__file__).parent

path_package = path_here / "libchess"
path_package.mkdir(exist_ok=True)

path_target = path_here.parent / f"cmake-build-release"
def copy_file(src: Path, dst: Path):
    if src.exists():
        dst.write_bytes(src.read_bytes())
    else:
        print(f"Warning: {src} does not exist and will be skipped.")
copy_file(path_target / "libchess.dll", path_package / "libchess.dll")
copy_file(path_target / "libchess.pyd", path_package / "libchess.pyd")
copy_file(path_here   / "libchess.pyi", path_package / "libchess.pyi")
copy_file(path_here   / "__init__.py",  path_package / "__init__.py")
(path_package / "py.typed").touch()

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