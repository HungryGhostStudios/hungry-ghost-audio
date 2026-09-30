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

function historyFixture(change={}) {
 const previous={signed:true,notarized:true,path,key,sha256,bytes:bytes.length};
 const nextHash='b'.repeat(64), nextBytes=new TextEncoder().encode('NEW-RELEASE-030');
 const nextKey=`macos/0.3.0/${nextHash}/HungryGhostSuite-0.3.0-macOS-Universal.pkg`;
 const current={signed:true,notarized:true,path:'/downloads/'+nextKey,key:nextKey,sha256:nextHash,bytes:nextBytes.length};
 const calls=[], objects=new Map([[key,{body:bytes,hash:sha256}],[nextKey,{body:nextBytes,hash:nextHash}]]);
 const downloads={macArtifact:current,macArtifacts:[{...previous,...change}]};
 const env={STORE_CONFIG:JSON.stringify({downloads}),RELEASES:{
  async head(k){calls.push(['head',k]);const o=objects.get(k);return o?{size:o.body.length,customMetadata:{sha256:o.hash}}:null;},
  async get(k,options){calls.push(['get',k,options]);const o=objects.get(k);if(!o)return null;const range=options?.range;
   return {size:o.body.length,customMetadata:{sha256:o.hash},body:range?o.body.slice(range.offset,range.offset+range.length):o.body};}
 }};
 return {calls,env,downloads,previous,current,objects,
  fetch:(p,headers={},method='GET')=>worker.fetch(new Request('https://hungryghostaudio.com'+p,{headers,method}),env)};
}

test('verified current and historical installers keep independent immutable downloads',async()=>{
 const f=historyFixture();
 for(const [artifact,content] of [[f.previous,'0123456789'],[f.current,'NEW-RELEASE-030']]){
  const response=await f.fetch(artifact.path);assert.equal(response.status,200);assert.equal(await response.text(),content);
  assert.equal(response.headers.get('ETag'),'"'+artifact.sha256+'"');
  assert.equal(response.headers.get('Content-Length'),String(artifact.bytes));
  assert.equal(response.headers.get('Cache-Control'),'public, max-age=31536000, immutable');
  assert.ok(response.headers.get('Content-Disposition').includes(artifact.key.split('/').at(-1)));
 }
 assert.deepEqual(f.calls.map(c=>c.slice(0,2)),[['head',f.previous.key],['get',f.previous.key],['head',f.current.key],['get',f.current.key]]);
 const head=await f.fetch(f.previous.path,{},'HEAD');assert.equal(head.status,200);assert.equal(await head.text(),'');
 assert.deepEqual(f.calls.at(-1),['head',f.previous.key]);
});

test('unknown and invalid historical artifacts fail before any storage access',async()=>{
 for(const change of [{signed:false},{notarized:false},{sha256:'bad'},{sha256:'c'.repeat(64)},
     {bytes:0},{bytes:1.5},{key:'macos/../'+key},{path:'/downloads/unrelated.pkg'}]){
  const f=historyFixture(change), response=await f.fetch(f.previous.path);
  assert.equal(response.status,404);assert.deepEqual(f.calls,[]);
 }
 const f=historyFixture();
 assert.equal((await f.fetch('/downloads/macos/0.4.0/'+sha256+'/HungryGhostSuite-0.4.0-macOS-Universal.pkg')).status,404);
 assert.deepEqual(f.calls,[]);
 for(const invalidHistory of [null,{},'all',[null,false,{}]]){
  f.env.STORE_CONFIG=JSON.stringify({downloads:{macArtifact:f.current,macArtifacts:invalidHistory}});
  assert.equal((await f.fetch(f.previous.path)).status,404);assert.deepEqual(f.calls,[]);
 }
});

test('historical paths retain exact range and conditional responses after current release changes',async()=>{
 for(const [range,body,contentRange] of [['bytes=2-5','2345','bytes 2-5/10'],['bytes=-3','789','bytes 7-9/10'],['bytes=7-','789','bytes 7-9/10']]){
  const f=historyFixture(), response=await f.fetch(f.previous.path,{Range:range,'If-Range':'"'+sha256+'"'});
  assert.equal(response.status,206);assert.equal(await response.text(),body);
  assert.equal(response.headers.get('Content-Range'),contentRange);
  assert.equal(response.headers.get('Content-Length'),String(body.length));
  assert.equal(f.calls[1][1],f.previous.key);
 }
 const f=historyFixture(), cached=await f.fetch(f.previous.path,{'If-None-Match':'"'+sha256+'"'});
 assert.equal(cached.status,304);assert.deepEqual(f.calls,[['head',f.previous.key]]);
 const stale=await f.fetch(f.previous.path,{Range:'bytes=5-','If-Range':'"'+f.current.sha256+'"'});
 assert.equal(stale.status,200);assert.equal(await stale.text(),'0123456789');
});

test('historical downloads retain storage guards and reject ambiguous allowlist paths',async()=>{
 const mismatch=historyFixture();mismatch.objects.get(key).hash='c'.repeat(64);
 assert.equal((await mismatch.fetch(path)).status,503);assert.deepEqual(mismatch.calls,[['head',key]]);
 const method=historyFixture();assert.equal((await method.fetch(path,{},'POST')).status,405);assert.deepEqual(method.calls,[]);
 const duplicate=historyFixture();duplicate.downloads.macArtifacts.push({...duplicate.previous,bytes:99});
 duplicate.env.STORE_CONFIG=JSON.stringify({downloads:duplicate.downloads});
 assert.equal((await duplicate.fetch(path)).status,404);assert.deepEqual(duplicate.calls,[]);
});
