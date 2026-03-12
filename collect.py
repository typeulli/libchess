from pathlib import Path

class Collect:
    def __init__(self, target: str) -> None:
        self.path_here = Path(__file__).parent
        self.path_target = self.path_here / target
        self.path_dst = self.path_target / "libchess"
        self.collection: list[Path] = []
        self.creation: list[tuple[Path, str]] = []
    def from_build(self, filename: str):
        self.collection.append(self.path_here / f"cmake-build-release/{filename}")
        return self
    def from_unique(self, filename: str):
        self.collection.append(self.path_target / filename)
        return self
    def from_data(self, filename: str, data: str = ""):
        self.creation.append((self.path_dst / filename, data))
        return self
    def __call__(self) -> None:
        self.path_dst.mkdir(exist_ok=True)
        for src in self.collection:
            dst = self.path_dst / src.name
            if src.exists():
                dst.write_bytes(src.read_bytes())
            else:
                print(f"Warning: {src} does not exist and will be skipped.")
        for dst, data in self.creation:
            if data == "":
                dst.touch()
            else:
                dst.write_text(data)

dictionary = {
    "python": Collect("python")
        .from_build("libchess.dll")
        .from_build("libchess.pyd")
        .from_unique("libchess.pyi")
        .from_data("py.typed")
        .from_data("__init__.py", "from .libchess import *"),
}

if __name__ == "__main__":
    import sys
    if len(sys.argv) != 2:
        print("Usage: collect.py [target]")
        print("Available targets:")
        for key in dictionary.keys():
            print(f"  {key}")
        sys.exit(1)
    target = sys.argv[1]
    if target not in dictionary:
        print(f"Error: target '{target}' is not defined.")
        print("Available targets:")
        for key in dictionary.keys():
            print(f"  {key}")
        sys.exit(1)
    dictionary[target]()