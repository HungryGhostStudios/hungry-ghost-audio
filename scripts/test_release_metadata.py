"""Release provenance must ignore gallery flags but reject altered build inputs."""
import copy
import json
import tempfile
import unittest
from argparse import Namespace
from pathlib import Path
from unittest.mock import patch
from release_metadata import product_version, suite_version, verify_native_source, bundle_name
from validate_release import preflight_versions, validate


class ReleaseMetadataTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix='hg-release-metadata-')
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.prior = [{'id': 'bond', 'version': '0.3.0', 'status': 'development',
                       'engine': 'BusCompressor', 'controls': [{'default': 2}], 'price': 5.99}]
        (self.root / 'CMakeLists.txt').write_text(
            'project(HungryGhostSuite VERSION 0.3.0 LANGUAGES C CXX)\n'
            'set(HG_REVERB_VERSION 0.3.1)\n'
            'set(HG_HAUNT_VERSION 0.1.3)\n'
            'set(HG_EFFECTS_VERSION 0.3.0)\n', encoding='utf-8')

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
        self.assertEqual(product_version('haunt', self.root), '0.1.3')
        self.assertEqual(bundle_name({'id':'haunt','name':'HAUNT'}), 'Hungry Ghost HAUNT Preview')

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


class WindowsVersionPreflightTests(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix='hg-pe-version-')
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name)
        self.product = {'id': 'bond', 'name': 'BOND', 'version': '0.3.0'}
        self.bundle = self.root / 'BOND_artefacts/Release/VST3/Hungry Ghost BOND.vst3'
        self.binary = self.bundle / 'Contents/x86_64-win/Hungry Ghost BOND.vst3'
        self.binary.parent.mkdir(parents=True)
        self.binary.write_bytes(b'Not loaded: the version API is mocked in these tests')
        self.info = self.bundle / 'Contents/Resources/moduleinfo.json'
        self.info.parent.mkdir(parents=True)
        self.info.write_text('{"Version":"0.3.0",}', encoding='utf-8')
        self.args = Namespace(build=self.root, output=self.root, validator=self.root / 'pluginval.exe')

    def test_matching_manifest_cannot_hide_stale_pe_version(self):
        with patch('validate_release.windows_product_version', return_value='0.2.0'), \
             patch('validate_release.subprocess.run') as run:
            result = validate(self.product, self.args)
            self.assertFalse(result['passed'])
            self.assertIn("ProductVersion '0.2.0'", result['error'])
            self.assertIn("catalogue '0.3.0'", result['error'])
            self.assertIn('regenerate JUCE generated RC', result['error'])
            run.assert_not_called()

    def test_missing_pe_version_fails_closed_before_pluginval(self):
        with patch('validate_release.windows_product_version', side_effect=ValueError('Missing Windows PE ProductVersion string')), \
             patch('validate_release.subprocess.run') as run:
            result = validate(self.product, self.args)
            self.assertFalse(result['passed'])
            self.assertIn('Missing Windows PE ProductVersion', result['error'])
            run.assert_not_called()

    def test_matching_pe_and_manifest_versions_pass_preflight(self):
        with patch('validate_release.windows_product_version', return_value='0.3.0'):
            self.assertEqual(preflight_versions(self.binary, self.info, '0.3.0'), '0.3.0')

    def test_stale_manifest_is_still_rejected(self):
        self.info.write_text('{"Version":"0.2.0",}', encoding='utf-8')
        with patch('validate_release.windows_product_version') as read_pe:
            with self.assertRaisesRegex(ValueError, 'moduleinfo version differs'):
                preflight_versions(self.binary, self.info, '0.3.0')
            read_pe.assert_not_called()


if __name__ == '__main__':
    unittest.main()
