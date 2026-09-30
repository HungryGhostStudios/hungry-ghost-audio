"""Release provenance must ignore gallery flags but reject altered build inputs."""
import copy
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch
from release_metadata import product_version, suite_version, verify_native_source


class ReleaseMetadataTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix='hg-release-metadata-')
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.prior = [{'id': 'bond', 'version': '0.3.0', 'status': 'development',
                       'engine': 'BusCompressor', 'controls': [{'default': 2}], 'price': 5.99}]
        (self.root / 'CMakeLists.txt').write_text(
            'project(HungryGhostSuite VERSION 0.3.0 LANGUAGES C CXX)\n'
            'set(HG_REVERB_VERSION 0.3.1)\n', encoding='utf-8')

    def check_source(self, current):
        (self.root / 'catalogue.json').write_text(json.dumps(current), encoding='utf-8')
        with patch('release_metadata.subprocess.run') as run, \
             patch('release_metadata.subprocess.check_output', return_value=json.dumps(self.prior)):
            verify_native_source('a' * 40, self.root)
            diff = run.call_args_list[1].args[0]
            self.assertIn('Source', diff)
            self.assertIn('Assets', diff)
            self.assertIn('Tests', diff)
            self.assertIn('scripts/release_metadata.py', diff)
            self.assertTrue(run.call_args_list[1].kwargs['check'])

    def test_authoritative_versions(self):
        self.assertEqual(suite_version(self.root), '0.3.0')
        self.assertEqual(product_version('reverb', self.root), '0.3.1')
        self.assertEqual(product_version('bond', self.root), '0.3.0')

    def test_gallery_and_validation_flags_may_change(self):
        current = copy.deepcopy(self.prior)
        current[0].update(status='validated', image='/assets/plugins/bond.png')
        self.check_source(current)

    def test_actual_product_fields_must_match(self):
        for field, value in [('version', '0.2.0'), ('id', 'rift'),
                             ('controls', [{'default': 4}]), ('price', 9.99),
                             ('engine', 'Compressor'), ('description', 'Changed')]:
            with self.subTest(field=field):
                current = copy.deepcopy(self.prior)
                current[0][field] = value
                with self.assertRaises(ValueError):
                    self.check_source(current)

    def test_untrusted_revision_is_rejected_before_git(self):
        with patch('release_metadata.subprocess.run') as run:
            with self.assertRaises(ValueError):
                verify_native_source('main; unexpected', self.root)
            run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
