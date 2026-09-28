import test from 'node:test';
import assert from 'node:assert/strict';
import worker from '../src/worker.mjs';
const sha256='a'.repeat(64), key=`macos/0.2.0/${sha256}/HungryGhostSuite-0.2.0-macOS-Universal.pkg`;
const path='/downloads/'+key, bytes=new TextEncoder().encode('0123456789');
function fixture(change={}, objectChange={}) {
 const calls=[];
 const artifact={signed:true,notarized:true,path,key,sha256,bytes:bytes.length,...change};
 const metadata={size:bytes.length,customMetadata:{sha256},...objectChange};
 const env={STORE_CONFIG:JSON.stringify({downloads:{macArtifact:artifact}}),RELEASES:{
  async head(k){calls.push(['head',k]);return metadata;},
  async get(k,options){calls.push(['get',k,options]);const range=options?.range;return {...metadata,body:range?bytes.slice(range.offset,range.offset+range.length):bytes};}
 }};
 return {calls,env,fetch:(headers={},method='GET',p=path)=>worker.fetch(new Request('https://hungryghostaudio.com'+p,{headers,method}),env)};
}
test('unpublished, unsigned or mismatched installers are never read from storage',async()=>{
 for(const change of [{signed:false},{notarized:false},{sha256:'bad'},{sha256:'b'.repeat(64)},{path:'/downloads/other'},{bytes:0},{key:'macos/../'+key}]){
  const f=fixture(change);assert.equal((await f.fetch()).status,404);assert.equal(f.calls.length,0);
 }
 const f=fixture();assert.equal((await f.fetch({},'GET','/downloads/unlisted.pkg')).status,404);
 assert.equal(f.calls.length,0);
 assert.equal((await worker.fetch(new Request('https://hungryghostaudio.com'+path),{})).status,404);
});
test('full download and HEAD describe the same immutable attachment',async()=>{
 const f=fixture(), response=await f.fetch();assert.equal(response.status,200);
 assert.equal(await response.text(),'0123456789');assert.equal(response.headers.get('Content-Length'),'10');
 assert.equal(response.headers.get('Accept-Ranges'),'bytes');assert.equal(response.headers.get('ETag'),'"'+sha256+'"');
 assert.match(response.headers.get('Content-Disposition'),/attachment; filename="HungryGhostSuite-0.2.0-macOS-Universal.pkg"/);
 assert.equal(response.headers.get('X-Content-Type-Options'),'nosniff');
 const h=fixture(), head=await h.fetch({},'HEAD');assert.equal(head.status,200);assert.equal(await head.text(),'');
 assert.equal(head.headers.get('Content-Length'),'10');assert.deepEqual(h.calls,[['head',key]]);
});
test('resumed, suffix and open-ended downloads return precisely the requested bytes',async()=>{
 for(const [range,body,contentRange] of [['bytes=2-5','2345','bytes 2-5/10'],['bytes=7-','789','bytes 7-9/10'],['bytes=-3','789','bytes 7-9/10'],['bytes=8-99','89','bytes 8-9/10'],['bytes=-99','0123456789','bytes 0-9/10']]){
  const f=fixture(), r=await f.fetch({Range:range});assert.equal(r.status,206);assert.equal(await r.text(),body);
  assert.equal(r.headers.get('Content-Range'),contentRange);assert.equal(r.headers.get('Content-Length'),String(body.length));
 }
});
test('invalid or multiple ranges fail without fetching a body',async()=>{
 for(const range of ['bytes=10-','bytes=7-2','bytes=-0','bytes=-','bytes=0-1,5-6','bytes=0-99999999999999999999','items=0-2']){
  const f=fixture(), r=await f.fetch({Range:range});assert.equal(r.status,416);assert.equal(r.headers.get('Content-Range'),'bytes */10');assert.deepEqual(f.calls,[['head',key]]);
 }
});
test('conditional requests avoid stale resume and unnecessary body reads',async()=>{
 const f=fixture(), r=await f.fetch({'If-None-Match':'"'+sha256+'"'});assert.equal(r.status,304);assert.deepEqual(f.calls,[['head',key]]);
 const stale=await fixture().fetch({Range:'bytes=5-','If-Range':'"old-release"'});assert.equal(stale.status,200);assert.equal(await stale.text(),'0123456789');
 const current=await fixture().fetch({Range:'bytes=5-','If-Range':'"'+sha256+'"'});assert.equal(current.status,206);assert.equal(await current.text(),'56789');
});
test('storage metadata mismatch and unsupported methods cannot serve an installer',async()=>{
 for(const metadata of [{size:9},{customMetadata:{sha256:'b'.repeat(64)}}]){
  const f=fixture({},metadata);assert.equal((await f.fetch()).status,503);assert.deepEqual(f.calls,[['head',key]]);
 }
 const f=fixture(), r=await f.fetch({},'POST');assert.equal(r.status,405);assert.equal(r.headers.get('Allow'),'GET, HEAD');assert.equal(f.calls.length,0);
 f.env.RELEASES.head=async()=>{throw Error('storage offline');};assert.equal((await f.fetch()).status,503);
});
test('range offsets above 2 GiB stay accurate without buffering the installer',async()=>{
 const f=fixture({bytes:2603406456},{size:2603406456});
 const r=await f.fetch({Range:'bytes=2400000000-2400000003'},'HEAD');assert.equal(r.status,206);
 assert.equal(r.headers.get('Content-Range'),'bytes 2400000000-2400000003/2603406456');assert.equal(r.headers.get('Content-Length'),'4');
 assert.deepEqual(f.calls,[['head',key]]);
});
