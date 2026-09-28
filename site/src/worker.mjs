export const json=(data,status=200)=>new Response(JSON.stringify(data),{status,headers:{'Content-Type':'application/json; charset=utf-8','Cache-Control':'no-store','X-Content-Type-Options':'nosniff'}});
import releaseConfiguration from './release-config.mjs';
import {releaseDownload} from './release-download.mjs';
export function configuration(env){try{return env.STORE_CONFIG?JSON.parse(env.STORE_CONFIG):releaseConfiguration;}catch{return {ready:false,products:{}};}}
export function checkoutDestination(value){const url=new URL(value);if(url.protocol!=='https:'||url.username||url.password||!((url.hostname==='polar.sh'&&url.pathname.startsWith('/checkout/'))||(url.hostname==='buy.polar.sh'&&/^\/polar_cl_[A-Za-z0-9]+$/.test(url.pathname))))throw Error('Invalid checkout destination');return url.href;}
export function b64url(bytes){let value='';for(const b of new Uint8Array(bytes))value+=String.fromCharCode(b);return btoa(value).replace(/\+/g,'-').replace(/\//g,'_').replace(/=+$/,'');}
function fromPem(pem){return Uint8Array.from(atob(pem.replace(/-----[^-]+-----/g,'').replace(/\s/g,'')),c=>c.charCodeAt(0));}
export async function signEntitlement(payload,pem){const key=await crypto.subtle.importKey('pkcs8',fromPem(pem),{name:'RSASSA-PKCS1-v1_5',hash:'SHA-256'},false,['sign']);const bytes=new TextEncoder().encode(JSON.stringify(payload));const signature=await crypto.subtle.sign('RSASSA-PKCS1-v1_5',key,bytes);return {payload:b64url(bytes),signature:b64url(signature)};}
const security={'X-Content-Type-Options':'nosniff','Referrer-Policy':'strict-origin-when-cross-origin','Permissions-Policy':'camera=(), microphone=(), geolocation=()','Content-Security-Policy':"default-src 'self'; img-src 'self'; script-src 'self'; style-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'self'; form-action 'self' https://polar.sh"};
async function readBody(request){if(Number(request.headers.get('Content-Length'))>8192)throw Error('Request too large');const text=await request.text();if(text.length>8192)throw Error('Request too large');return JSON.parse(text);}
export async function api(request,env){
 const url=new URL(request.url),config=configuration(env);
 if(url.pathname==='/api/config'&&request.method==='GET')return json({ready:!!config.ready,products:config.products||{},downloads:config.downloads||{},support:config.support||'',source:config.source||''});
 if(request.method!=='POST')return json({error:'Not found'},404);
 const origin=request.headers.get('Origin');if(origin&&origin!==url.origin&&origin!==env.PUBLIC_ORIGIN)return json({error:'Origin not allowed'},403);
 let body;try{body=await readBody(request);}catch{return json({error:'Invalid request'},400);}
 if(url.pathname==='/api/checkout'){
  const product=body.product==='suite'?config.suite:config.products?.[body.product];
  if(!product?.ready||!product.checkout)return json({error:'This release is not available for purchase yet.'},409);
  try{return json({url:checkoutDestination(product.checkout)});}catch{return json({error:'Checkout configuration unavailable'},503);}
 }
 if(url.pathname==='/api/license/activate'||url.pathname==='/api/license/validate'){
  if(!env.LICENSE_SIGNING_KEY||!config.organizationId)return json({error:'Activation service is not ready'},503);
  if(typeof body.key!=='string'||body.key.length<8||body.key.length>200||!/^[a-f0-9]{64}$/.test(body.device||''))return json({error:'A valid licence key and device ID are required.'},400);
  const validate=url.pathname.endsWith('/validate')||!!body.activationId;
  if(validate&&(typeof body.activationId!=='string'||body.activationId.length>100))return json({error:'A valid activation ID is required.'},400);
  const fields={key:body.key,organization_id:config.organizationId,...(validate?{activation_id:body.activationId}:{label:'Hungry Ghost '+body.device.substring(0,12),meta:{device:body.device}})};
  if(validate&&!body.activationId)return json({error:'Activation ID is required.'},400);
  let response;try{response=await fetch('https://api.polar.sh/v1/customer-portal/license-keys/'+(validate?'validate':'activate'),{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(fields),signal:AbortSignal.timeout(10000)});}catch{return json({error:'Licence provider is temporarily unavailable'},502);}
  if(!response.ok)return json({error:response.status===422?'The key could not be activated. Check the key and available device activations.':'The licence could not be verified.'},response.status===422?422:403);
  const result=await response.json(),licence=result.license_key||result;
  if(licence.organization_id!==config.organizationId||licence.status!=='granted')return json({error:'Licence is not active.'},403);
  const activation=validate?result.activation:result;
  if(!activation?.id||activation.license_key_id!==licence.id||activation?.meta?.device!==body.device||(validate&&activation.id!==body.activationId))return json({error:'This activation belongs to another device.'},403);
  const entitlement=config.benefits?.[licence.benefit_id];if(!Array.isArray(entitlement)||!entitlement.length)return json({error:'This key does not grant a Hungry Ghost Audio product.'},403);
  const now=Math.floor(Date.now()/1000);let expires=now+90*86400;if(licence.expires_at)expires=Math.min(expires,Math.floor(new Date(licence.expires_at).getTime()/1000));if(!Number.isFinite(expires)||expires<=now)return json({error:'Licence has expired.'},403);
  const payload={v:1,device:body.device,products:entitlement,issued:now,expires,activationId:activation.id,licenceId:licence.id};
  return json(await signEntitlement(payload,env.LICENSE_SIGNING_KEY));
 }
 return json({error:'Not found'},404);
}
export default {async fetch(request,env){const url=new URL(request.url);if(url.hostname==='www.hungryghostaudio.com')return Response.redirect('https://hungryghostaudio.com'+url.pathname+url.search,308);let response;if(url.pathname.startsWith('/api/')){try{response=await api(request,env);}catch{return json({error:'Service temporarily unavailable'},503);}}else if(url.pathname.startsWith('/downloads/')){try{response=await releaseDownload(request,env,configuration(env));}catch{return json({error:'Download temporarily unavailable'},503);}}else response=await env.ASSETS.fetch(request);const headers=new Headers(response.headers);for(const [key,value] of Object.entries(security))headers.set(key,value);return new Response(response.body,{status:response.status,headers});}};
