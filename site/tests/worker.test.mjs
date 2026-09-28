import test from 'node:test';import assert from 'node:assert/strict';import {generateKeyPairSync,verify} from 'node:crypto';import worker,{api,signEntitlement,configuration,checkoutDestination} from '../src/worker.mjs';
test('purchase gates remain closed without a validated release',async()=>{const response=await api(new Request('https://hungryghostaudio.com/api/checkout',{method:'POST',body:JSON.stringify({product:'suite'})}),{STORE_CONFIG:JSON.stringify({ready:false,suite:{ready:false}})});assert.equal(response.status,409);});
test('the approved release exposes 51 distinct checkouts with matching licence coverage',async()=>{const config=configuration({}),ids=Object.keys(config.products);assert.equal(config.ready,true);assert.equal(ids.length,50);const entries=[['suite',config.suite],...Object.entries(config.products)];assert.equal(new Set(entries.map(([,p])=>p.productId)).size,51);assert.equal(new Set(entries.map(([,p])=>p.checkout)).size,51);assert.equal(Object.keys(config.benefits).length,51);for(const [id,p] of entries){assert.equal(p.ready,true);const response=await api(new Request('https://hungryghostaudio.com/api/checkout',{method:'POST',body:JSON.stringify({product:id})}),{});assert.equal(response.status,200);assert.equal((await response.json()).url,checkoutDestination(p.checkout));}const coverage=Object.values(config.benefits);assert.deepEqual(coverage.find(x=>x.length===50).slice().sort(),ids.slice().sort());assert.deepEqual(coverage.filter(x=>x.length===1).flat().sort(),ids.slice().sort());assert.equal((await api(new Request('https://hungryghostaudio.com/api/checkout',{method:'POST',body:'{"product":"unknown"}'}),{})).status,409);});
test('www requests permanently redirect to the canonical storefront',async()=>{const response=await worker.fetch(new Request('https://www.hungryghostaudio.com/plugins?group=space'),{});assert.equal(response.status,308);assert.equal(response.headers.get('location'),'https://hungryghostaudio.com/plugins?group=space');});
test('checkout only accepts a configured Polar destination',async()=>{for(const checkout of ['https://evil.test/checkout/hello','http://polar.sh/checkout/a','https://polar.sh/unrelated']){const env={STORE_CONFIG:JSON.stringify({suite:{ready:true,checkout}})};assert.equal((await api(new Request('https://hungryghostaudio.com/api/checkout',{method:'POST',body:'{"product":"suite"}'}),env)).status,503);}const response=await api(new Request('https://hungryghostaudio.com/api/checkout',{method:'POST',body:'{"product":"suite"}'}),{STORE_CONFIG:JSON.stringify({suite:{ready:true,checkout:'https://polar.sh/checkout/test'}})});assert.equal(response.status,200);assert.equal((await response.json()).url,'https://polar.sh/checkout/test');});
test('cross-origin purchase requests are rejected',async()=>{assert.equal((await api(new Request('https://hungryghostaudio.com/api/checkout',{method:'POST',headers:{Origin:'https://evil.test'},body:'{}'}),{})).status,403);});
test('signed entitlements interoperate with standard RSA SHA-256',async()=>{const {privateKey,publicKey}=generateKeyPairSync('rsa',{modulusLength:3072});const payload={v:1,device:'a'.repeat(64),products:['feral'],issued:1,expires:2,activationId:'a',licenceId:'l'};const signed=await signEntitlement(payload,privateKey.export({type:'pkcs8',format:'pem'}));assert.equal(verify('sha256',Buffer.from(signed.payload,'base64url'),publicKey,Buffer.from(signed.signature,'base64url')),true);assert.equal(verify('sha256',Buffer.from(JSON.stringify({...payload,products:['suite']})),publicKey,Buffer.from(signed.signature,'base64url')),false);});
test('malformed public configuration fails closed',()=>assert.equal(configuration({STORE_CONFIG:'oops'}).ready,false));
test('real Polar checkout-link URLs are accepted without allowing lookalike domains',()=>{assert.equal(checkoutDestination('https://buy.polar.sh/polar_cl_Ab123'),'https://buy.polar.sh/polar_cl_Ab123');for(const value of ['https://buy.polar.sh.evil.test/polar_cl_Ab123','https://buy.polar.sh/unrelated','http://buy.polar.sh/polar_cl_Ab123','https://user:pass@buy.polar.sh/polar_cl_Ab123'])assert.throws(()=>checkoutDestination(value));});

const signingKeys=generateKeyPairSync('rsa',{modulusLength:3072});
const device='a'.repeat(64), org='11e9ccad-193e-4352-8255-080f0e219750', benefit='82bdc898-4d7b-4006-9990-91e2bcef628f';
const env={LICENSE_SIGNING_KEY:signingKeys.privateKey.export({type:'pkcs8',format:'pem'}),STORE_CONFIG:JSON.stringify({organizationId:org,benefits:{[benefit]:['feral','rift']}})};
const licence={id:'licence-id',organization_id:org,status:'granted',benefit_id:benefit,expires_at:null};
const activation={id:'activation-id',license_key_id:'licence-id',meta:{device}};
const request=(path='activate',body={key:'HG_test_key',device})=>new Request('https://hungryghostaudio.com/api/license/'+path,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(body)});
async function provider(result,work,status=200){const previous=globalThis.fetch;try{globalThis.fetch=async(url,options)=>{assert.match(url,/^https:\/\/api\.polar\.sh\/v1\/customer-portal\/license-keys\/(activate|validate)$/);const fields=JSON.parse(options.body);assert.equal(fields.organization_id,org);assert.ok(options.signal);return new Response(JSON.stringify(result),{status});};return await work();}finally{globalThis.fetch=previous;}}
test('activation grants only the configured benefit with a verifiable device signature',async()=>{
 const response=await provider({...activation,license_key:licence},()=>api(request(),env));assert.equal(response.status,200);
 const signed=await response.json();assert.equal(verify('sha256',Buffer.from(signed.payload,'base64url'),signingKeys.publicKey,Buffer.from(signed.signature,'base64url')),true);
 const payload=JSON.parse(Buffer.from(signed.payload,'base64url'));assert.deepEqual(payload.products,['feral','rift']);assert.equal(payload.device,device);assert.equal(payload.activationId,activation.id);assert.equal(payload.expires-payload.issued,90*86400);
});
test('refresh uses Polar validation and preserves a shorter paid expiry',async()=>{
 const expiry=Math.floor(Date.now()/1000)+86400;
 const response=await provider({...licence,expires_at:new Date(expiry*1000).toISOString(),activation},()=>api(request('validate',{key:'HG_test_key',device,activationId:activation.id}),env));
 assert.equal(response.status,200);const signed=await response.json();assert.equal(JSON.parse(Buffer.from(signed.payload,'base64url')).expires,expiry);
});
test('wrong organization, revoked status and unknown benefits fail closed',async()=>{
 for(const change of [{organization_id:'another-org'},{status:'revoked'},{status:'disabled'},{benefit_id:'unknown'},{expires_at:'invalid-date'},{expires_at:'2020-01-01T00:00:00Z'}]){
  assert.equal((await provider({...activation,license_key:{...licence,...change}},()=>api(request(),env))).status,403);
 }
});
test('wrong device, licence relationship and activation ID cannot grant a cache',async()=>{
 for(const change of [{meta:{device:'b'.repeat(64)}},{license_key_id:'other-key'},{id:'another-activation'},{id:null}]){
  assert.equal((await provider({...licence,activation:{...activation,...change}},()=>api(request('validate',{key:'HG_test_key',device,activationId:activation.id}),env))).status,403);
 }
});
test('malformed licence requests do not contact Polar',async()=>{
 const previous=globalThis.fetch;try{globalThis.fetch=()=>{throw Error('Unexpected provider call');};
 for(const body of [{key:'short',device},{key:'HG_test_key',device:'raw-hardware-id'},{key:'HG_test_key',device,activationId:{bad:true}}])assert.equal((await api(request('activate',body),env)).status,400);
 assert.equal((await api(request('validate'),env)).status,400);
 assert.equal((await api(request(),{})).status,503);
 }finally{globalThis.fetch=previous;}
});
test('Polar outage cannot issue an entitlement',async()=>assert.equal((await provider({error:'NotPermitted'},()=>api(request(),env),403)).status,403));
