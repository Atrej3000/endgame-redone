#!/usr/bin/env python3
"""Repository usage integrity check -- see docs/unused-code-assets-audit.md.

Deterministic, dependency-light (Python 3 standard library only), run from the
repository root. Two checks:

1. Asset-path integrity: extracts every `resource/...`-shaped string literal
   from src/*.c, confirms the file exists with EXACT case (a manual,
   case-sensitive directory-listing comparison -- not os.path.exists, which
   would silently succeed on a case-insensitive filesystem like Windows'
   default even when the requested case doesn't match the file on disk).
   Reports missing paths and casing mismatches with source file:line.
2. Dangling-prototype check: confirms every non-static function prototype
   declared in inc/header.h has a matching definition somewhere in src/*.c.

Neither check is proof against dynamic behavior this script cannot see (e.g.
a path built at runtime via string concatenation) -- as of this writing no
such construction exists anywhere in this codebase (confirmed by manual
audit), so static extraction is dispositive here, but this script does not
claim to prove that in general. Supports an explicit ALLOWLIST for paths or
symbols that are known-fine despite not matching a literal/definition search
(e.g. license files, documentation screenshots, packaging-only resources,
intentionally dynamic references) -- currently empty, since none are needed
after the Phase 7 cleanup; add entries here if a future addition needs one,
with a comment explaining why.

Usage: python scripts/audit_repository_usage.py
Exit code: 0 if all checks pass, 1 if any finding is reported.
"""
import re
import sys
from pathlib import Path

REPO_ROOT = Path(__file__).resolve().parent.parent
SRC_DIR = REPO_ROOT / "src"
HEADER_FILE = REPO_ROOT / "inc" / "header.h"

# Paths (relative to repo root, forward slashes) that are known-fine despite
# not being found by this script's static checks. These 3 are a CONFIRMED,
# documented, pre-existing case-mismatch bug (docs/unused-code-assets-audit.md
# section 8): the source requests lowercase-leading filenames but the files
# on disk are capitalized. They resolve today only because Windows'
# filesystem is case-insensitive, and would fail IMG_Load on case-sensitive
# macOS/Linux, this project's stated primary target. Allowlisted here (not
# silently ignored) so this script's other checks stay a meaningful gate
# rather than always failing on a known issue -- fixing the casing is a
# distinct bug-fix change, out of Phase 7's "remove unused content" charter,
# left for a dedicated future phase.
ALLOWLIST_ASSET_PATHS = {
    "resource/images/background/Sunset_front.png",
    "resource/images/terrain/brick_block.png",
    "resource/images/terrain/copper_block.png",
}

# Function names declared in header.h that are known-fine despite this
# script not finding a matching definition (e.g. a platform-specific
# definition guarded by an #ifdef this script doesn't evaluate). Empty as of
# Phase 7 -- every declared prototype has exactly one unconditional
# definition.
ALLOWLIST_PROTOTYPES = set()

ASSET_LITERAL_RE = re.compile(r'"([^"]*resource/[^"]*)"')
PROTOTYPE_RE = re.compile(
    r'^\s*(?:[A-Za-z_][\w\s\*]*?)\s+([A-Za-z_]\w*)\s*\([^;{}]*\)\s*;\s*$'
)


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


def check_asset_paths():
    findings = []
    for c_file, line_no, raw, normalized in find_asset_references():
        if normalized in ALLOWLIST_ASSET_PATHS:
            continue
        if not case_sensitive_exists(normalized):
            # Distinguish "missing entirely" from "exists, wrong case" for a
            # clearer report.
            case_insensitive_hit = (REPO_ROOT / normalized).exists()
            kind = "casing mismatch" if case_insensitive_hit else "missing path"
            findings.append(
                f"{c_file}:{line_no}: {kind} -- literal {raw!r} "
                f"(normalized: {normalized})"
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
    findings = []
    definitions = find_definitions()
    for name in find_prototypes():
        if name in ALLOWLIST_PROTOTYPES:
            continue
        if name not in definitions:
            findings.append(f"inc/header.h: prototype '{name}' has no matching definition in src/*.c")
    return findings


def main():
    asset_findings = check_asset_paths()
    prototype_findings = check_dangling_prototypes()

    if asset_findings:
        print("=== Asset-path integrity findings ===")
        for f in asset_findings:
            print(f)
        print()

    if prototype_findings:
        print("=== Dangling-prototype findings ===")
        for f in prototype_findings:
            print(f)
        print()

    total = len(asset_findings) + len(prototype_findings)
    if total == 0:
        print("audit_repository_usage: all checks passed (0 findings)")
        return 0
    print(f"audit_repository_usage: {total} finding(s)")
    return 1


if __name__ == "__main__":
    sys.exit(main())
