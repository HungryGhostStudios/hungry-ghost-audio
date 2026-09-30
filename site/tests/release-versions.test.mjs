import test from 'node:test';
import assert from 'node:assert/strict';
import {releaseVersions, productVersionSummary} from '../public/release-versions.js';

function fixture() {
 return {downloads:{
  installer:'https://github.com/example/releases/download/v0.3.0/HungryGhostSuite-0.3.0-Setup.exe',
  suite:'https://github.com/example/releases/download/v0.3.0/HungryGhostSuite-0.3.0-Windows-VST3.zip',
  macInstaller:'https://hungryghostaudio.com/downloads/macos/0.2.0/hash/HungryGhostSuite-0.2.0-macOS-Universal.pkg',
  macArtifact:{path:'/downloads/macos/0.2.0/hash/HungryGhostSuite-0.2.0-macOS-Universal.pkg'}
 },products:{
  bond:{download:'https://github.com/example/HungryGhost-BOND-0.3.0-Windows-VST3.zip'},
  reverb:{download:'https://github.com/example/HungryGhost-REVERB-0.3.1-Windows-VST3.zip'}
 }};
}

test('mixed platform rollout labels the offered Mac release separately from Windows',()=>{
 const config=fixture();
 assert.deepEqual(releaseVersions(config,'bond'),{windowsSuite:'0.3.0',windowsZip:'0.3.0',windowsPlugin:'0.3.0',macSuite:'0.2.0'});
 assert.equal(productVersionSummary(config,'bond'),'Windows plugin v0.3.0 · macOS suite v0.2.0');
 // Publishing only the Windows URL must not advance the displayed Mac version.
 config.products.bond.download='https://github.com/example/HungryGhost-BOND-0.4.0-Windows-VST3.zip';
 assert.equal(productVersionSummary(config,'bond'),'Windows plugin v0.4.0 · macOS suite v0.2.0');
 config.downloads.suite='https://github.com/example/HungryGhostSuite-0.2.0-Windows-VST3.zip';
 assert.equal(releaseVersions(config).windowsZip,'0.2.0');
});

test('REVERB retains its own plugin version while suite installer versions remain independent',()=>{
 const config=fixture();
 assert.equal(productVersionSummary(config,'reverb'),'Windows plugin v0.3.1 · macOS suite v0.2.0');
 config.downloads.macInstaller='https://hungryghostaudio.com/downloads/macos/0.3.0/hash/HungryGhostSuite-0.3.0-macOS-Universal.pkg';
 assert.equal(releaseVersions(config,'reverb').macSuite,'0.3.0');
 assert.equal(releaseVersions(config,'reverb').windowsSuite,'0.3.0');
});

test('an alias uses verified artifact metadata, but a versioned download URL takes precedence',()=>{
 const config=fixture();
 config.downloads.macInstaller='/downloads/mac';
 assert.equal(releaseVersions(config).macSuite,'0.2.0');
 config.downloads.macInstaller='/downloads/HungryGhostSuite-0.3.0-macOS-Universal.pkg?download=1';
 assert.equal(releaseVersions(config).macSuite,'0.3.0');
});

test('unavailable or unrecognized artifacts never invent a version or advertise Mac',()=>{
 assert.equal(productVersionSummary({products:{bond:{version:'9.9.9'}}},'bond'),'Windows plugin');
 const config=fixture();
 delete config.downloads.macInstaller;
 assert.equal(productVersionSummary(config,'bond'),'Windows plugin v0.3.0');
 config.downloads.installer='https://example.com/9.9.9/unrecognized.exe';
 delete config.downloads.suite;
 assert.equal(releaseVersions(config).windowsSuite,'');
});
