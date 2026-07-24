#!/usr/bin/env python3
"""Focused regression tests for audit_repository_usage.py."""
import tempfile
import unittest
from pathlib import Path

import audit_repository_usage as audit


class DanglingPrototypeCoverageTest(unittest.TestCase):
    def test_focused_multiline_header_requires_non_static_definition(self):
        original_root = audit.REPO_ROOT
        original_inc = audit.INC_DIR
        original_src = audit.SRC_DIR
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            inc = root / "inc"
            src = root / "src"
            inc.mkdir()
            src.mkdir()
            (inc / "focused.h").write_text(
                "bool focused_only_api(\n"
                "    int value);\n",
                encoding="utf-8",
            )
            try:
                audit.REPO_ROOT = root
                audit.INC_DIR = inc
                audit.SRC_DIR = src

                missing = audit.check_dangling_prototypes()
                self.assertEqual(len(missing.errors), 1)
                self.assertIn("focused.h:1", missing.errors[0])

                (src / "implementation.c").write_text(
                    "static bool focused_only_api(\n"
                    "    int value)\n"
                    "{\n"
                    "    return value != 0;\n"
                    "}\n",
                    encoding="utf-8",
                )
                static_only = audit.check_dangling_prototypes()
                self.assertEqual(len(static_only.errors), 1)

                (src / "implementation.c").write_text(
                    "bool focused_only_api(\n"
                    "    int value)\n"
                    "{\n"
                    "    return value != 0;\n"
                    "}\n",
                    encoding="utf-8",
                )
                defined = audit.check_dangling_prototypes()
                self.assertEqual(defined.errors, [])
            finally:
                audit.REPO_ROOT = original_root
                audit.INC_DIR = original_inc
                audit.SRC_DIR = original_src


if __name__ == "__main__":
    unittest.main()
