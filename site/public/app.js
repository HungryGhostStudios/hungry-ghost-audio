import {releaseVersions, productVersionSummary} from './release-versions.js';
import {track} from './analytics.js';

const $ = s => document.querySelector(s);
let products=[], family='All', config={ready:false,products:{}};
const macAvailable = id => Boolean(config.downloads?.macInstaller) && (!id || !config.downloads.macProducts || config.downloads.macProducts.includes(id));
const platformSummary = id => macAvailable(id) ? 'Windows x64 · Mac Intel / Apple Silicon · VST3 / AU' : 'Windows x64 · VST3';
const money = value => new Intl.NumberFormat('en-US',{style:'currency',currency:'USD',minimumFractionDigits:2,maximumFractionDigits:2}).format(value);
const safe = s => String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
const notice = message => {$('#notice').textContent=message;$('#notice').hidden=false;setTimeout(()=>$('#notice').hidden=true,9000);};
const categories = {
 All: {description:'The complete collection',shape:'<path d="M3 3h5v5H3zm9 0h5v5h-5zM3 12h5v5H3zm9 0h5v5h-5z"/>'},
 Vocal: {description:'Tune pitch and preserve expression',shape:'<path d="M3 10h2l2-6 3 12 3-9 2 3h2"/>'},
 Dynamics: {description:'Control impact and level',shape:'<path d="M2 10h3l2-6 4 12 2-6h5"/>'},
 Tone: {description:'Shape the frequency balance',shape:'<path d="M5 3v14M10 3v14M15 3v14M3 7h4m1 6h4m1-8h4"/>'},
 Colour: {description:'Add grit and harmonics',shape:'<path d="M4 16V9m6 7V3m6 13V6M2 17h16"/>'},
 Space: {description:'Build depth and atmosphere',shape:'<circle cx="10" cy="10" r="7"/><circle cx="10" cy="10" r="3"/><path d="M10 1v2m0 14v2M1 10h2m14 0h2"/>'},
 Motion: {description:'Bring the sound to life',shape:'<path d="M2 10c3-12 5 12 8 0s5 12 8 0"/>'},
 Stereo: {description:'Place and widen the image',shape:'<path d="M3 5h5v10H3zm9 0h5v10h-5zM9 10h2"/>'},
 Utility: {description:'The essentials for every session',shape:'<path d="M4 4h12v12H4zM10 7v6m-3-3h6"/>'}
};
const categoryIcon = name => `<svg class="category-icon" viewBox="0 0 20 20" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round" aria-hidden="true" focusable="false">${categories[name]?.shape||categories.All.shape}</svg>`;
function renderCategoryFilters(){
 document.querySelectorAll('[data-family]').forEach(button=>{
  const name=button.dataset.family, count=name==='All'?products.length:products.filter(p=>p.family===name).length;
  button.classList.add('category-filter');button.dataset.category=name;
  button.setAttribute('aria-label',`${name==='All'?'All tools':name}, ${count} tools`);
  button.innerHTML=`<span class="category-filter-top">${categoryIcon(name)}<strong>${name==='All'?'All tools':safe(name)}</strong><span class="category-count">${count}</span></span><span class="category-filter-description">${safe(categories[name].description)}</span>`;
 });
}
function productCard(p){
 return `<article class="product-card" data-category="${safe(p.family)}"><div class="card-top"><span class="category-badge">${categoryIcon(p.family)}${safe(p.family)}</span><span class="product-index">${String(products.indexOf(p)+1).padStart(2,'0')} / ${macAvailable(p.id)?'VST3 / AU':'VST3'}</span></div><h4>${safe(p.name)}</h4><p>${safe(p.description)}</p>${p.earlyAccess?'<p class="small-note">EARLY ACCESS · LIVE VOCAL CORRECTION</p>':''}<div class="card-bottom"><span>${money(p.price)}</span><span class="build-state">${config.products[p.id]?.ready?'EXPLORE ↗':p.status==='validated'?'VALIDATED BUILD ↗':'IN DEVELOPMENT ↗'}</span></div><button aria-label="Explore ${safe(p.name)}" data-product="${safe(p.id)}">Explore ${safe(p.name)}</button></article>`;
}
function render(){
 renderCategoryFilters();
 const term=$('#search').value.toLowerCase().trim();
 const shown=products.filter(p=>(family==='All'||p.family===family)&&`${p.name} ${p.family} ${p.description}`.toLowerCase().includes(term));
 $('#result-count').textContent=`${shown.length} ${shown.length===1?'tool':'tools'}${family==='All'?'':` in ${family.toLowerCase()}`}`;
 $('#empty').hidden=shown.length!==0;
 $('#product-grid').innerHTML=Object.keys(categories).filter(name=>name!=='All').map(name=>{
  const group=shown.filter(p=>p.family===name);if(!group.length)return '';
  return `<section class="category-group" data-category="${safe(name)}" aria-labelledby="category-${name.toLowerCase()}"><div class="category-heading"><span class="category-heading-icon">${categoryIcon(name)}</span><div><h3 id="category-${name.toLowerCase()}">${safe(name)}</h3><p>${safe(categories[name].description)}</p></div><span class="category-total">${group.length} ${group.length===1?'tool':'tools'}</span></div><div class="product-grid">${group.map(productCard).join('')}</div></section>`;
 }).join('');
}
async function checkout(id, button){
 const old=button.textContent;button.disabled=true;button.textContent='Opening secure checkout…';
 try{const response=await fetch('/api/checkout',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({product:id})});const result=await response.json();if(!response.ok)throw Error(result.error||'Checkout is temporarily unavailable.');const destination=new URL(result.url);if(destination.protocol!=='https:'||!['polar.sh','buy.polar.sh'].includes(destination.hostname))throw Error('Invalid checkout destination.');track('checkout_opened',{product:id});window.location.assign(result.url);}catch(error){notice(error.message);button.disabled=false;button.textContent=old;}
}
function openProduct(id){
 const p=products.find(p=>p.id===id);if(!p)return;const release=config.products[p.id];
 $('#product-dialog').dataset.category=p.family;
 const controls=p.controls.length?p.controls.map(c=>c.name):p.id==='feral'?['Eight EQ bands','Per-band dynamics','Mid / side','External key','Bus compressor','A / B']:['Room / Chamber / Hall / Plate / Cloud','Decay','Damping','Motion','Duck','Freeze'];
 $('#dialog-content').innerHTML=`<p class="eyebrow">${safe(p.family.toUpperCase())} / HUNGRY GHOST AUDIO</p><h2 class="dialog-title" id="dialog-title">${safe(p.name)}</h2><p class="dialog-description">${safe(p.description)}</p>${p.image?`<img class="dialog-image" src="${safe(p.image)}" alt="Actual ${safe(p.name)} Windows plugin interface">`:''}<div class="control-list">${controls.map(c=>`<span>${safe(c)}</span>`).join('')}</div><p class="small-note">${platformSummary(p.id)}<br>${safe(productVersionSummary(config,p.id))} · AGPLv3 source</p>${p.releaseNotes?`<p class="small-note">${safe(p.releaseNotes)}</p>`:''}<div class="dialog-bottom"><div><strong>${money(p.price)}</strong><p>Perpetual purchase · Two devices<br>Taxes calculated at checkout.</p></div><button class="button solid" id="dialog-buy" ${release?.ready?'':'disabled'}>${release?.ready?'Buy '+safe(p.name)+' ↗':'Purchasing opens soon'}</button></div>${release?.download?`<p class="trial-link"><a class="button solid" href="${safe(release.download)}">Try Windows VST3 ↗</a>${macAvailable(p.id)?` <a class="button solid" href="${safe(config.downloads.macInstaller)}">Try Mac VST3 / AU ↗</a><span class="small-note">Choose ${safe(p.name)} in the Mac installer’s Customize panel.</span>`:''}</p>`:`<p class="small-note">${p.status==='validated'?'The native build has passed our current test suite. Store licensing and delivery are being prepared.':'This processor is in development. Purchasing will open after its build and release are validated.'}</p>`}`;
 if(release?.ready)$('#dialog-buy').addEventListener('click',e=>checkout(p.id,e.currentTarget));
 $('#product-dialog').showModal();
}
document.querySelectorAll('[data-family]').forEach(b=>b.addEventListener('click',()=>{family=b.dataset.family;document.querySelectorAll('[data-family]').forEach(x=>x.setAttribute('aria-pressed',String(x===b)));render();}));
$('#search').addEventListener('input',render);
$('#product-grid').addEventListener('click',e=>{const b=e.target.closest('[data-product]');if(b)openProduct(b.dataset.product);});
document.querySelectorAll('[data-open]').forEach(a=>a.addEventListener('click',e=>{e.preventDefault();openProduct(a.dataset.open);}));
$('.dialog-close').addEventListener('click',()=>$('#product-dialog').close());
$('#product-dialog').addEventListener('click',e=>{if(e.target===$('#product-dialog')){const b=e.target.getBoundingClientRect();if(e.clientX<b.left||e.clientX>b.right||e.clientY<b.top||e.clientY>b.bottom)e.target.close();}});
$('#buy-suite').addEventListener('click',e=>checkout('suite',e.currentTarget));
try{const responses=await Promise.all([fetch('/catalogue.json'),fetch('/api/config')]);if(!responses[0].ok)throw Error('The collection could not be loaded.');products=await responses[0].json();if(responses[1].ok)config=await responses[1].json();render();if(config.ready){$('#buy-suite').disabled=false;$('#buy-suite').textContent='Get the complete suite ↗';$('#suite-status').textContent='Secure checkout by Polar. Licence key delivered with your purchase.';}if(config.downloads?.installer&&config.downloads?.suite){$('#downloads').hidden=false;$('#download-installer').href=config.downloads.installer;$('#download-zip').href=config.downloads.suite;$('#support-copy').innerHTML='<a class="text-link" href="https://github.com/HungryGhostStudios/hungry-ghost-audio/releases">Release downloads and checksums ↗</a><br><a class="text-link" href="https://github.com/HungryGhostStudios/hungry-ghost-audio/blob/main/Docs/UserGuide.md">Installation and user guide ↗</a><br><a class="text-link" href="https://polar.sh/hungry-ghost-audio/portal">Purchase receipts, keys and devices ↗</a>';if(!config.ready)$('#buy-suite').textContent='Purchasing opens soon';if(!config.ready)$('#suite-status').textContent='The tested release is available to try. Purchasing opens when store setup is complete.';}if(macAvailable()){$('#platform-spec').textContent='Windows + macOS · VST3 / AU';$('#suite-platforms').textContent=`Windows: ${products.length} tools · Mac: ${config.downloads.macProducts?.length||products.length} tools`;$('#download-description').textContent=`Start a 30-day trial. Windows includes ${products.length} tools; the current Mac installer includes ${config.downloads.macProducts?.length||products.length}. HAUNT is early access—check its product listing for platform availability.`;$('#download-mac').hidden=false;$('#download-mac').href=config.downloads.macInstaller;$('#download-zip').textContent='Windows manual install ZIP ↗';$('#download-platforms').innerHTML='Windows 10 / 11 · x64 · VST3<br>macOS 11+ · Intel / Apple Silicon · VST3 + AU<br>Source, release notes and checksums on GitHub.';$('#supported-platforms').textContent='Windows 10 / 11 x64 supports VST3. macOS 11 and later supports universal VST3 and Audio Units on Intel and Apple Silicon. Choose Audio Units for Logic Pro. The same purchase covers both platforms. See the release notes for host checks.';}if(config.source)$('#source-link').href=config.source;if(config.support){$('#support-copy').append(document.createTextNode(` For support, contact ${config.support}.`));}}catch(error){$('#result-count').textContent='Unable to load the collection.';notice(error.message);}

// Installer labels follow the configured artifacts, including staggered platform releases.
const versions=releaseVersions(config);
if(config.downloads?.installer)$('#download-installer').textContent=`Windows installer${versions.windowsSuite?' · v'+versions.windowsSuite:''} ↗`;
if(config.downloads?.suite)$('#download-zip').textContent=`Windows manual install ZIP${versions.windowsZip?' · v'+versions.windowsZip:''} ↗`;
if(macAvailable())$('#download-mac').textContent=`Mac universal installer${versions.macSuite?' · v'+versions.macSuite:''} ↗`;
