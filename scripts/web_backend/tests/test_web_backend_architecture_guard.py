from __future__ import annotations

import pathlib
import subprocess
import sys
import unittest


REPO_ROOT = pathlib.Path(__file__).resolve().parents[3]


class WebBackendArchitectureGuardTest(unittest.TestCase):
    def test_architecture_guard_passes(self):
        result = subprocess.run(
            [sys.executable, str(REPO_ROOT / 'scripts' / 'check_web_backend_architecture.py')],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )

        self.assertEqual(result.returncode, 0, msg=result.stdout + result.stderr)
        self.assertIn('passed', result.stdout.lower())


if __name__ == '__main__':
    unittest.main()
