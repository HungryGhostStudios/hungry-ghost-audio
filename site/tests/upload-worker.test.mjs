import test from 'node:test';
import assert from 'node:assert/strict';
import {createHash} from 'node:crypto';
import uploader from '../../scripts/macos/upload-worker.mjs';
const body=new TextEncoder().encode('signed installer fixture'), sha=createHash('sha256').update(body).digest('hex');
function fixture(){
 const calls=[],key=`macos/0.2.0/${sha}/HungryGhostSuite-0.2.0-macOS-Universal.pkg`;
 const part={partNumber:1,etag:'part-etag'};let stored=false;
 const metadata={size:body.length,customMetadata:{sha256:sha}};
 const upload={uploadId:'upload-fixture',async uploadPart(n,b){calls.push(['part',n,new Uint8Array(b)]);return part;},async complete(parts){assert.deepEqual(parts,[part]);stored=true;},async abort(){calls.push(['abort']);}};
 const env={ARTIFACT_BYTES:String(body.length),ARTIFACT_SHA256:sha,ARTIFACT_KEY:key,EXPIRES_AT:String(Date.now()+60000),UPLOAD_TOKEN:'fixture-private-token',RELEASES:{
  async head(k){assert.equal(k,key);return stored?metadata:null;},
  async createMultipartUpload(k,options){assert.equal(k,key);assert.equal(options.customMetadata.sha256,sha);calls.push(['start']);return upload;},
  resumeMultipartUpload(k,id){assert.equal(k,key);assert.equal(id,'upload-fixture');return upload;},
  async get(k){assert.equal(k,key);return stored?{...metadata,body}:null;}
 }};
 const fetch=(path,options={})=>uploader.fetch(new Request('https://upload.test'+path,{...options,headers:{Authorization:'Bearer fixture-private-token',...options.headers}}),env);
 return {calls,env,fetch};
}
test('ephemeral upload service rejects missing, wrong and expired credentials',async()=>{
 const f=fixture();for(const Authorization of ['', 'Bearer wrong'])assert.equal((await f.fetch('/start',{method:'POST',headers:{Authorization}})).status,404);
 f.env.EXPIRES_AT=String(Date.now()-1);assert.equal((await f.fetch('/start',{method:'POST'})).status,404);assert.deepEqual(f.calls,[]);
});
test('multipart transfer checks every part and exposes the exact completed object privately',async()=>{
 const f=fixture();assert.equal((await f.fetch('/start',{method:'POST'})).status,200);
 const options={method:'PUT',body,headers:{'Content-Length':String(body.length),'X-Part-SHA256':sha}};
 assert.equal((await f.fetch('/part/1?uploadId=upload-fixture',{...options,headers:{...options.headers,'X-Part-SHA256':'b'.repeat(64)}})).status,400);
 assert.equal((await f.fetch('/part/2?uploadId=upload-fixture',options)).status,400);
 assert.equal((await f.fetch('/part/1?uploadId=upload-fixture',options)).status,200);
 assert.equal((await f.fetch('/finish?uploadId=upload-fixture',{method:'POST',body:JSON.stringify([{partNumber:2,etag:'wrong'}])})).status,400);
 assert.equal((await f.fetch('/finish?uploadId=upload-fixture',{method:'POST',body:JSON.stringify([{partNumber:1,etag:'part-etag'}])})).status,200);
 const object=await f.fetch('/artifact');assert.equal(object.status,200);assert.equal(createHash('sha256').update(new Uint8Array(await object.arrayBuffer())).digest('hex'),sha);
 assert.equal((await f.fetch('/start',{method:'POST'})).status,409);
});
