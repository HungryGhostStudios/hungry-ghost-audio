// Describe the artifacts actually offered, independently of the newer gallery.
function filename(url) {
 try { return decodeURIComponent(new URL(url, 'https://hungryghostaudio.com').pathname).split('/').at(-1); }
 catch { return ''; }
}
const suiteVersion = url => /^HungryGhostSuite-(\d+\.\d+\.\d+)-(?:Setup\.exe|Windows-VST3\.zip|macOS-Universal\.pkg)$/.exec(filename(url))?.[1] || '';
const pluginVersion = url => /^HungryGhost-[A-Z0-9]+-(\d+\.\d+\.\d+)-Windows-VST3\.zip$/.exec(filename(url))?.[1] || '';

export function releaseVersions(config, productId) {
 const downloads=config.downloads || {};
 return {
  windowsSuite: suiteVersion(downloads.installer) || suiteVersion(downloads.suite),
  windowsZip: suiteVersion(downloads.suite),
  windowsPlugin: pluginVersion(config.products?.[productId]?.download),
  macSuite: downloads.macInstaller && (!productId || !downloads.macProducts || downloads.macProducts.includes(productId)) ? suiteVersion(downloads.macInstaller) || suiteVersion(downloads.macArtifact?.path) : ''
 };
}

export function productVersionSummary(config, productId) {
 const versions=releaseVersions(config, productId);
 const windows=versions.windowsPlugin ? `Windows plugin v${versions.windowsPlugin}` : 'Windows plugin';
 if(!config.downloads?.macInstaller)return windows;
 if(config.downloads.macProducts && !config.downloads.macProducts.includes(productId))return windows+' · macOS not yet available';
 return windows + ' · ' + (versions.macSuite ? `macOS suite v${versions.macSuite}` : 'macOS suite');
}
