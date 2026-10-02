const campaignKeys=['utm_source','utm_medium','utm_campaign','utm_content'];
const current=Object.fromEntries(campaignKeys.map(key=>[key,new URLSearchParams(location.search).get(key)||'']).filter(([,value])=>value));
let campaign=current;
try{
 if(Object.keys(current).length)sessionStorage.setItem('hg_campaign',JSON.stringify(current));
 else campaign=JSON.parse(sessionStorage.getItem('hg_campaign')||'{}');
}catch{}

export function track(event,details={}){
 const payload=JSON.stringify({event,path:location.pathname,...campaign,...details});
 if(navigator.sendBeacon){
  const sent=navigator.sendBeacon('/api/analytics',new Blob([payload],{type:'application/json'}));
  if(sent)return;
 }
 fetch('/api/analytics',{method:'POST',headers:{'Content-Type':'application/json'},body:payload,keepalive:true}).catch(()=>{});
}

const listened=new Set();
const listening=document.querySelector('#listen');
if(listening){
 const mark=()=>{if(listened.has('listen'))return;listened.add('listen');track('listen_view');};
 if(location.hash==='#listen')mark();
 if('IntersectionObserver' in window){
  const observer=new IntersectionObserver(entries=>{if(entries.some(entry=>entry.isIntersecting)){mark();observer.disconnect();}},{threshold:.35});
  observer.observe(listening);
 }
}

document.addEventListener('click',event=>{
 const link=event.target.closest('a[href]');if(!link)return;
 const href=link.getAttribute('href')||'';
 if(!href.startsWith('/downloads/'))return;
 const label=link.id==='download-mac'?'mac_installer':link.id==='download-zip'?'windows_zip':'windows_installer';
 const product=link.closest('#product-dialog')?.querySelector('.dialog-title')?.textContent?.toLowerCase().replace(/[^a-z0-9]+/g,'-').replace(/(^-|-$)/g,'')||'suite';
 track('trial_download',{product,label});
});
