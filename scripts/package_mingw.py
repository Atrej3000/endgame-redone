#!/usr/bin/env python3
"""Stage a self-contained Windows/MinGW game directory.

Only the shipped executable, its four SDL runtime DLLs, runtime assets, and
project documentation are copied. Test executables and build logs are never
included.
"""

from __future__ import annotations

import argparse
import hashlib
import os
from pathlib import Path
import shutil
import tempfile


RUNTIME_DLLS = ("SDL2.dll", "SDL2_image.dll", "SDL2_ttf.dll", "SDL2_mixer.dll")
RUNTIME_RESOURCE_DIRECTORIES = ("images", "sounds", "text")
PACKAGE_MARKER = ".endgame-package"
PACKAGE_MARKER_CONTENT = "endgame-redone Windows package v1\n"
PACKAGE_ROOT_FILES = {
    PACKAGE_MARKER,
    "endgame.exe",
    *RUNTIME_DLLS,
    "LICENSE",
    "README.md",
}


def parse_args() -> argparse.Namespace:
    repository = Path(__file__).resolve().parent.parent
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=repository / "build-mingw",
        help="directory containing endgame-mingw.exe and the SDL runtime DLLs",
    )
    parser.add_argument(
        "--destination",
        required=True,
        type=Path,
        help="new runnable package directory",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="replace an existing package created by this script",
    )
    parser.add_argument(
        "--verify",
        action="store_true",
        help="verify an existing package without modifying it",
    )
    args = parser.parse_args()
    if args.force and args.verify:
        parser.error("--force and --verify cannot be used together")
    return args


def contains(parent: Path, child: Path) -> bool:
    try:
        child.relative_to(parent)
    except ValueError:
        return False
    return True


def validate_destination(repository: Path, build_dir: Path, destination: Path) -> None:
    if destination == destination.parent:
        raise ValueError("refusing to package into a filesystem root")
    if (
        destination == repository
        or contains(destination, repository)
        or contains(repository, destination)
    ):
        raise ValueError(
            "destination must be outside the repository and must not be one of its parents"
        )
    if (
        destination == build_dir
        or contains(destination, build_dir)
        or contains(build_dir, destination)
    ):
        raise ValueError("destination must not be the build directory or one of its parents")
    protected = (repository / "resource", repository / "src", repository / "inc", repository / "scripts")
    if any(destination == path or contains(path, destination) for path in protected):
        raise ValueError("destination must not be inside a source or resource directory")


def validate_inputs(repository: Path, build_dir: Path) -> list[Path]:
    required_files = [
        build_dir / "endgame-mingw.exe",
        *(build_dir / name for name in RUNTIME_DLLS),
        repository / "LICENSE",
        repository / "README.md",
    ]
    missing = [path for path in required_files if not path.is_file()]
    for directory_name in RUNTIME_RESOURCE_DIRECTORIES:
        directory = repository / "resource" / directory_name
        if not directory.is_dir():
            missing.append(directory)
    if missing:
        formatted = "\n".join(f"  - {path}" for path in missing)
        raise FileNotFoundError(f"cannot create package; required inputs are missing:\n{formatted}")
    return required_files


def file_digest(path: Path) -> tuple[int, str]:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return path.stat().st_size, digest.hexdigest()


def tree_manifest(root: Path) -> dict[str, tuple[int, str]]:
    return {
        path.relative_to(root).as_posix(): file_digest(path)
        for path in sorted(root.rglob("*"))
        if path.is_file()
    }


def verify_resource_tree(package: Path, repository: Path) -> None:
    packaged_resource = package / "resource"
    resource_entries = {path.name for path in packaged_resource.iterdir()}
    expected_entries = set(RUNTIME_RESOURCE_DIRECTORIES)
    if resource_entries != expected_entries:
        missing = sorted(expected_entries - resource_entries)
        extra = sorted(resource_entries - expected_entries)
        raise ValueError(
            f"package resource directories differ (missing={missing}, extra={extra})"
        )

    for directory_name in RUNTIME_RESOURCE_DIRECTORIES:
        source_root = repository / "resource" / directory_name
        package_root = packaged_resource / directory_name
        source_manifest = tree_manifest(source_root)
        package_manifest = tree_manifest(package_root)
        if not source_manifest:
            raise ValueError(f"runtime resource source is empty: {source_root}")
        if package_manifest != source_manifest:
            source_files = set(source_manifest)
            package_files = set(package_manifest)
            missing = sorted(source_files - package_files)
            extra = sorted(package_files - source_files)
            changed = sorted(
                name
                for name in source_files & package_files
                if source_manifest[name] != package_manifest[name]
            )
            raise ValueError(
                f"package resource/{directory_name} differs from source "
                f"(missing={missing}, extra={extra}, changed={changed})"
            )


def verify_package(package: Path, repository: Path, build_dir: Path) -> None:
    if not package.is_dir():
        raise FileNotFoundError(f"package directory does not exist: {package}")
    if any(path.is_symlink() for path in package.rglob("*")):
        raise ValueError("package must not contain symbolic links")

    marker = package / PACKAGE_MARKER
    if not marker.is_file() or marker.read_text(encoding="utf-8") != PACKAGE_MARKER_CONTENT:
        raise ValueError(f"not an endgame package (valid {PACKAGE_MARKER} is missing)")

    required_files = [
        package / "endgame.exe",
        *(package / name for name in RUNTIME_DLLS),
        package / "LICENSE",
        package / "README.md",
    ]
    missing = [path for path in required_files if not path.is_file()]
    if not (package / "resource").is_dir():
        missing.append(package / "resource")
    if missing:
        formatted = "\n".join(f"  - {path}" for path in missing)
        raise FileNotFoundError(f"package is incomplete:\n{formatted}")

    root_files = {path.name for path in package.iterdir() if path.is_file()}
    root_directories = {path.name for path in package.iterdir() if path.is_dir()}
    if root_files != PACKAGE_ROOT_FILES or root_directories != {"resource"}:
        raise ValueError(
            "package root differs from the release layout "
            f"(files={sorted(root_files)}, directories={sorted(root_directories)})"
        )
    verify_resource_tree(package, repository)
    for document in ("LICENSE", "README.md"):
        if file_digest(package / document) != file_digest(repository / document):
            raise ValueError(f"package {document} differs from repository source")
    build_artifacts = {
        "endgame.exe": build_dir / "endgame-mingw.exe",
        **{dll: build_dir / dll for dll in RUNTIME_DLLS},
    }
    missing_build_artifacts = [
        source for source in build_artifacts.values() if not source.is_file()
    ]
    if missing_build_artifacts:
        formatted = "\n".join(f"  - {path}" for path in missing_build_artifacts)
        raise FileNotFoundError(
            "cannot verify package binaries; build artifacts are missing:\n"
            f"{formatted}"
        )
    for package_name, source in build_artifacts.items():
        if file_digest(package / package_name) != file_digest(source):
            raise ValueError(
                f"package {package_name} differs from build artifact {source.name}"
            )

    executable_names = {path.relative_to(package).as_posix() for path in package.rglob("*.exe")}
    if executable_names != {"endgame.exe"}:
        raise ValueError(f"package contains unexpected executables: {sorted(executable_names)}")
    logs = [path for path in package.rglob("*.log") if path.is_file()]
    if logs:
        formatted = "\n".join(f"  - {path}" for path in logs)
        raise ValueError(f"package contains build logs:\n{formatted}")


def validate_replaceable_destination(destination: Path) -> None:
    if not destination.is_dir():
        raise ValueError(
            f"refusing to replace non-package destination: {destination}"
        )
    marker = destination / PACKAGE_MARKER
    if not marker.is_file() or marker.read_text(encoding="utf-8") != PACKAGE_MARKER_CONTENT:
        raise ValueError(
            f"refusing to replace {destination}; valid {PACKAGE_MARKER} is missing"
        )


def stage_package(repository: Path, build_dir: Path, staging: Path) -> None:
    shutil.copy2(build_dir / "endgame-mingw.exe", staging / "endgame.exe")
    for dll in RUNTIME_DLLS:
        shutil.copy2(build_dir / dll, staging / dll)

    resource_destination = staging / "resource"
    resource_destination.mkdir()
    for directory_name in RUNTIME_RESOURCE_DIRECTORIES:
        shutil.copytree(
            repository / "resource" / directory_name,
            resource_destination / directory_name,
        )

    shutil.copy2(repository / "LICENSE", staging / "LICENSE")
    shutil.copy2(repository / "README.md", staging / "README.md")
    (staging / PACKAGE_MARKER).write_text(PACKAGE_MARKER_CONTENT, encoding="utf-8")


def main() -> int:
    args = parse_args()
    repository = Path(__file__).resolve().parent.parent
    build_dir = args.build_dir.resolve()
    if args.destination.is_symlink():
        raise ValueError("destination must not be a symbolic link")
    destination = args.destination.resolve()

    validate_destination(repository, build_dir, destination)
    if args.verify:
        verify_package(destination, repository, build_dir)
        print(f"verified runnable Windows package: {destination}")
        return 0

    validate_inputs(repository, build_dir)
    if destination.exists() and not args.force:
        raise FileExistsError(
            f"destination already exists: {destination} (pass --force to replace it)"
        )
    if destination.exists():
        validate_replaceable_destination(destination)

    destination.parent.mkdir(parents=True, exist_ok=True)
    temporary = Path(
        tempfile.mkdtemp(prefix=f".{destination.name}.staging-", dir=destination.parent)
    )
    try:
        stage_package(repository, build_dir, temporary)
        verify_package(temporary, repository, build_dir)
        if destination.exists():
            shutil.rmtree(destination)
        os.replace(temporary, destination)
    finally:
        if temporary.exists():
            shutil.rmtree(temporary)

    print(f"packaged runnable Windows build: {destination}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
