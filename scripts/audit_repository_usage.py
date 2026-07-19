#!/usr/bin/env python3
"""Repository usage integrity check -- see docs/unused-code-assets-audit.md and
docs/asset-path-portability.md.

Deterministic, dependency-light (Python 3 standard library only), run from the
repository root. Three checks:

1. Asset-path integrity: extracts every `resource/...`-shaped string literal
   from src/*.c and validates it against the real tracked files:
   - confirms the file exists with EXACT case, component by component (a
     manual, case-sensitive directory-listing comparison -- not
     os.path.exists/Path.exists(), which would silently succeed on a
     case-insensitive filesystem like Windows' default even when the
     requested case doesn't match the file on disk);
   - confirms the resolved path is a regular file, not a directory;
   - confirms no ambiguous case-colliding sibling exists in the same
     directory (e.g. both `Foo.png` and `foo.png` present -- inherently
     unsafe to resolve consistently across case-sensitive and
     case-insensitive filesystems);
   - confirms the raw literal contains no Windows-only backslash path
     separator;
   - reports (informationally, never as an error) when two or more distinct
     string literals resolve to the same canonical file.
2. Dangling-prototype check: confirms every non-static function prototype
   declared in inc/header.h has a matching definition somewhere in src/*.c.
3. Duplicate-declaration check (added Phase 10): confirms no function name is
   declared as a prototype in more than one inc/*.h file. header.h and the
   focused headers (scene.h, frame.h, entity_spawn.h, input_command.h, app.h)
   are each meant to be the single authoritative source for their own
   declarations -- a regression guard against reintroducing an accidental
   duplicate like the one found and fixed in Phase 10 (doRender's prototype
   was, for unrelated historical reasons, duplicated twice within header.h
   itself; see docs/gamestate-decomposition.md section 5).

None of these checks are proof against dynamic behavior this script cannot see (e.g.
a path built at runtime via string concatenation) -- as of this writing no
such construction exists anywhere in this codebase (confirmed by manual
audit, docs/asset-path-portability.md), so static extraction is dispositive
here, but this script does not claim to prove that in general.

Findings are classified into three tiers:
- ERRORS: cause a non-zero exit code. A real problem with no accepted
  exception.
- KNOWN EXCEPTIONS: an asset path or prototype explicitly listed in the
  allowlists below. Reported, but do not fail the build -- use only for a
  documented, deliberate, currently-empty-by-default reason (see the
  allowlist comments). A real, undocumented mismatch is always an ERROR,
  never silently downgraded.
- INFORMATIONAL: never fail the build (e.g. duplicate aliases to the same
  file). Reported for visibility only.

Usage: python scripts/audit_repository_usage.py
Exit code: 0 if (asset errors + prototype errors + duplicate-declaration
errors) == 0, 1 otherwise -- regardless of how many known exceptions or
informational notices exist.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"
INC_DIR = REPO_ROOT / "inc"
HEADER_FILE = REPO_ROOT / "inc" / "header.h"

# Paths (relative to repo root, forward slashes) that are known-fine despite
# not matching this script's normal checks. Empty as of Phase 8 -- the three
# case-mismatch bugs previously listed here were corrected (see
# docs/asset-path-portability.md) rather than permanently excepted. Add an
# entry here only for a genuinely deliberate, documented exception (e.g. a
# path only ever reached through a runtime-constructed string this script
# cannot see) -- never to silence an undiagnosed mismatch.
ALLOWLIST_ASSET_PATHS = set()

# Function names declared in header.h that are known-fine despite this
# script not finding a matching definition (e.g. a platform-specific
# definition guarded by an #ifdef this script doesn't evaluate). Empty as of
# Phase 7 -- every declared prototype has exactly one unconditional
# definition.
ALLOWLIST_PROTOTYPES = set()

# Function names known-fine despite being declared as a prototype in more
# than one inc/*.h file (e.g. a deliberate, documented transition-period
# duplicate while callers migrate to a focused header -- see
# docs/solid-gof-audit.md section 7.3 for the app_change_scene precedent from
# Phase 9, since resolved in Phase 10). Empty as of Phase 10 -- every
# declared prototype now has exactly one declaring header.
ALLOWLIST_DUPLICATE_DECLARATIONS = set()

ASSET_LITERAL_RE = re.compile(r'"([^"]*resource/[^"]*)"')
PROTOTYPE_RE = re.compile(
    r'^\s*(?:[A-Za-z_][\w\s\*]*?)\s+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*;\s*$'
)


class Findings:
    """Accumulates errors / known exceptions / informational notices for one
    check, each a list of human-readable strings."""

    def __init__(self):
        self.errors = []
        self.known_exceptions = []
        self.info = []


def find_asset_references():
    """Yield (file, line_no, raw_literal, normalized_path) for every
    resource/-shaped string literal in src/*.c."""
    for c_file in sorted(SRC_DIR.glob("*.c")):
        text = c_file.read_text(encoding="utf-8", errors="replace")
        for line_no, line in enumerate(text.splitlines(), start=1):
            for match in ASSET_LITERAL_RE.finditer(line):
                raw = match.group(1)
                # Normalize: keep from the first "resource/" onward, so
                # "./resource/x.png" and "../resource/x.png" both become
                # "resource/x.png", checked relative to the repo root.
                idx = raw.find("resource/")
                normalized = raw[idx:]
                yield c_file.relative_to(REPO_ROOT), line_no, raw, normalized


def case_sensitive_exists(rel_path):
    """Check that rel_path exists under REPO_ROOT with every path component
    matching on-disk casing exactly, regardless of the host filesystem's own
    case sensitivity."""
    current = REPO_ROOT
    for part in Path(rel_path).parts:
        if not current.is_dir():
            return False
        try:
            entries = {p.name for p in current.iterdir()}
        except OSError:
            return False
        if part not in entries:
            return False
        current = current / part
    return current.exists()


def case_colliding_sibling(rel_path):
    """If rel_path's exact-case entry exists, check whether any OTHER entry
    in the same directory differs only by case (e.g. both Foo.png and
    foo.png present) -- an inherently ambiguous, unsafe-to-resolve-
    consistently situation across case-sensitive and case-insensitive
    filesystems. Returns the colliding sibling's name, or None."""
    full_path = REPO_ROOT / rel_path
    parent = full_path.parent
    if not parent.is_dir():
        return None
    target_name = full_path.name
    target_lower = target_name.lower()
    try:
        entries = [p.name for p in parent.iterdir()]
    except OSError:
        return None
    for name in entries:
        if name != target_name and name.lower() == target_lower:
            return name
    return None


def check_asset_paths():
    findings = Findings()
    canonical_to_sites = {}  # normalized_path -> [ "file:line", ... ]

    for c_file, line_no, raw, normalized in find_asset_references():
        site = f"{c_file}:{line_no}"

        if "\\" in raw:
            findings.errors.append(
                f"{site}: Windows-only backslash path separator in literal {raw!r}"
            )
            continue

        if normalized in ALLOWLIST_ASSET_PATHS:
            findings.known_exceptions.append(
                f"{site}: allowlisted exception for {normalized!r} (see ALLOWLIST_ASSET_PATHS comment)"
            )
            continue

        if not case_sensitive_exists(normalized):
            case_insensitive_hit = (REPO_ROOT / normalized).exists()
            kind = "casing mismatch" if case_insensitive_hit else "missing path"
            findings.errors.append(
                f"{site}: {kind} -- literal {raw!r} (normalized: {normalized})"
            )
            continue

        full_path = REPO_ROOT / normalized
        if not full_path.is_file():
            findings.errors.append(
                f"{site}: resolved path {normalized!r} exists but is not a regular file"
            )
            continue

        sibling = case_colliding_sibling(normalized)
        if sibling is not None:
            findings.errors.append(
                f"{site}: {normalized!r} has an ambiguous case-colliding sibling "
                f"({sibling!r} in the same directory)"
            )
            continue

        canonical_to_sites.setdefault(normalized, []).append(site)

    for normalized, sites in canonical_to_sites.items():
        if len(sites) > 1:
            findings.info.append(
                f"{normalized!r} is referenced from {len(sites)} sites (harmless duplicate alias): "
                + ", ".join(sites)
            )

    return findings


def find_prototypes():
    """Yield function names declared as prototypes in header.h."""
    text = HEADER_FILE.read_text(encoding="utf-8", errors="replace")
    for line in text.splitlines():
        match = PROTOTYPE_RE.match(line)
        if match:
            yield match.group(1)


def find_definitions():
    """Return the set of function names that have what looks like a
    definition (a line ending in the argument list with no trailing
    semicolon, i.e. not a prototype or a call statement) somewhere in
    src/*.c."""
    definitions = set()
    # Accepts both brace-on-next-line and brace-on-same-line (K&R) styles --
    # a definition's argument-list line never ends in ';' (that's what
    # distinguishes it from a prototype or a call statement), but the line
    # may or may not be followed immediately by '{'.
    def_re = re.compile(r'^\s*(?:static\s+)?(?:[A-Za-z_][\w\s\*]*?)\s+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{?\s*$')
    for c_file in sorted(SRC_DIR.glob("*.c")):
        text = c_file.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            match = def_re.match(line)
            if match:
                definitions.add(match.group(1))
    return definitions


def check_dangling_prototypes():
    findings = Findings()
    definitions = find_definitions()
    for name in find_prototypes():
        site = "inc/header.h"
        if name in ALLOWLIST_PROTOTYPES:
            findings.known_exceptions.append(
                f"{site}: allowlisted exception for prototype '{name}' (see ALLOWLIST_PROTOTYPES comment)"
            )
            continue
        if name not in definitions:
            findings.errors.append(
                f"{site}: prototype '{name}' has no matching definition in src/*.c"
            )
    return findings


def find_header_prototypes():
    """Yield (header_file, function_name) for every prototype-shaped line in
    every inc/*.h file (not just header.h) -- deterministic, regex-based,
    reusing PROTOTYPE_RE; no C parser."""
    for h_file in sorted(INC_DIR.glob("*.h")):
        text = h_file.read_text(encoding="utf-8", errors="replace")
        for line in text.splitlines():
            match = PROTOTYPE_RE.match(line)
            if match:
                yield h_file.relative_to(REPO_ROOT), match.group(1)


def check_duplicate_declarations():
    """Confirm no function name is declared as a prototype in more than one
    inc/*.h file -- each header is meant to be the single authoritative
    source for its own declarations. Matching by name alone (not full
    signature text) is sufficient: C does not allow overloading, so the same
    name declared twice across headers is always either a redundant
    duplicate or a conflicting declaration the compiler would already
    reject -- both worth flagging here."""
    findings = Findings()
    name_to_files = {}
    for h_file, name in find_header_prototypes():
        name_to_files.setdefault(name, set()).add(h_file)

    for name, files in sorted(name_to_files.items()):
        if len(files) <= 1:
            continue
        file_list = ", ".join(str(f) for f in sorted(files))
        if name in ALLOWLIST_DUPLICATE_DECLARATIONS:
            findings.known_exceptions.append(
                f"'{name}' declared in {len(files)} headers ({file_list}) -- "
                f"allowlisted exception (see ALLOWLIST_DUPLICATE_DECLARATIONS comment)"
            )
            continue
        findings.errors.append(
            f"'{name}' declared as a prototype in {len(files)} headers: {file_list}"
        )
    return findings


def _print_section(title, items):
    if not items:
        return
    print(f"=== {title} ===")
    for item in items:
        print(item)
    print()


def main():
    asset = check_asset_paths()
    proto = check_dangling_prototypes()
    dupes = check_duplicate_declarations()

    _print_section("Asset path errors", asset.errors)
    _print_section("Asset path known exceptions", asset.known_exceptions)
    _print_section("Asset path informational notices", asset.info)
    _print_section("Prototype errors", proto.errors)
    _print_section("Prototype known exceptions", proto.known_exceptions)
    _print_section("Duplicate declaration errors", dupes.errors)
    _print_section("Duplicate declaration known exceptions", dupes.known_exceptions)

    asset_error_count = len(asset.errors)
    proto_error_count = len(proto.errors)
    dupe_error_count = len(dupes.errors)
    known_exception_count = (
        len(asset.known_exceptions) + len(proto.known_exceptions) + len(dupes.known_exceptions)
    )
    info_count = len(asset.info)

    print(f"Asset path errors: {asset_error_count}")
    print(f"Prototype errors: {proto_error_count}")
    print(f"Duplicate declaration errors: {dupe_error_count}")
    print(f"Known exceptions: {known_exception_count}")
    print(f"Informational notices: {info_count}")

    total_errors = asset_error_count + proto_error_count + dupe_error_count
    if total_errors > 0:
        print("Result: FAIL")
        return 1
    if known_exception_count > 0:
        print("Result: PASS WITH KNOWN EXCEPTIONS")
        return 0
    print("Result: PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main())
