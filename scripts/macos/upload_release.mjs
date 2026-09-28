// Upload only a successful, signed and notarized release. Wrangler uses the
// operator's existing authentication; no permanent R2 credential is created.
import {createHash,randomBytes} from 'node:crypto';
import {createReadStream} from 'node:fs';
import {mkdtemp,readFile,writeFile,copyFile,open,rm,stat} from 'node:fs/promises';
import {tmpdir} from 'node:os';
import {resolve,dirname,join,sep} from 'node:path';
import {fileURLToPath} from 'node:url';
import {spawn} from 'node:child_process';
const root=resolve(dirname(fileURLToPath(import.meta.url)),'../..');
const packagePath=resolve(process.argv[2]||''), releaseDirectory=dirname(packagePath);
if(!process.argv[2] || !/^HungryGhostSuite-[A-Za-z0-9._-]+-macOS-Universal\.pkg$/.test(packagePath.split(/[\\/]/).at(-1)))throw Error('Supply the final Mac installer path');
const manifest=JSON.parse(await readFile(join(releaseDirectory,'macOS-manifest.json'),'utf8'));
const notarization=JSON.parse(await readFile(join(releaseDirectory,'notarization.json'),'utf8'));
if(manifest.signing!=='Developer ID signed, notarized and stapled' || notarization.status!=='Accepted'
 || manifest.products?.length!==100 || manifest.products.some(p=>p.passed!==true||!p.architectures?.includes('arm64')||!p.architectures?.includes('x86_64')))
 throw Error('Signed, notarized universal release evidence is required');
const hashFile=async(file)=>{const hash=createHash('sha256');for await(const chunk of createReadStream(file))hash.update(chunk);return hash.digest('hex');};
const sha256=await hashFile(packagePath), bytes=(await stat(packagePath)).size;
if(sha256!==manifest.installerSHA256)throw Error('Final installer checksum differs from release manifest');
const version=manifest.suiteVersion;
if(!/^[A-Za-z0-9._-]+$/.test(version))throw Error('Invalid release version');
const key=`macos/${version}/${sha256}/${packagePath.split(/[\\/]/).at(-1)}`;
const name='hungry-ghost-audio-upload-'+randomBytes(5).toString('hex'), token=randomBytes(32).toString('hex');
const scratch=await mkdtemp(join(tmpdir(),'hungryghost-release-upload-'));
const configPath=join(scratch,'wrangler.json');
const wrangler=resolve(root,'site/node_modules/wrangler/bin/wrangler.js');
function cli(args,input){return new Promise((done,reject)=>{
 const child=spawn(process.execPath,[wrangler,...args,'--config',configPath],{cwd:resolve(root,'site'),stdio:['pipe','pipe','pipe'],windowsHide:true});
 let output='';child.stdout.on('data',x=>output+=x);child.stderr.on('data',()=>{});
 child.on('error',()=>reject(Error('Release hosting command could not start')));
 child.on('close',code=>code===0?done(output):reject(Error('Release hosting command failed: '+args[0])));child.stdin.end(input||'');
});}
let deployed=false, base='', uploadId='', complete=false;
try {
 await copyFile(join(root,'scripts/macos/upload-worker.mjs'),join(scratch,'worker.mjs'));
 await writeFile(configPath,JSON.stringify({name,main:'worker.mjs',compatibility_date:'2026-09-27',workers_dev:true,
  r2_buckets:[{binding:'RELEASES',bucket_name:'hungry-ghost-audio-releases'}],
  vars:{ARTIFACT_KEY:key,ARTIFACT_BYTES:String(bytes),ARTIFACT_SHA256:sha256,EXPIRES_AT:String(Date.now()+2*60*60*1000)}}),'utf8');
 const output=await cli(['deploy']);deployed=true;
 base=output.match(/https:\/\/[a-z0-9.-]+\.workers\.dev/)?.[0];if(!base)throw Error('Upload service URL was not returned');
 await cli(['secret','put','UPLOAD_TOKEN'],token);
 const request=async(path,options={})=>{
  const {timeoutMs=300000,acceptStatuses=[],...fetchOptions}=options;
  const response=await fetch(base+path,{...fetchOptions,headers:{Authorization:'Bearer '+token,...fetchOptions.headers},signal:AbortSignal.timeout(timeoutMs)});
  if(!response.ok&&!acceptStatuses.includes(response.status))throw Error('Release transfer failed with HTTP '+response.status);return response;
 };
 // Secret deployment may reach the edge shortly after Wrangler acknowledges it.
 let startResponse;
 for(let attempt=0;attempt<12;attempt++){
  startResponse=await request('/start',{method:'POST',acceptStatuses:[404,409]});
  if(startResponse.status!==404)break;
  await new Promise(done=>setTimeout(done,5000));
 }
 if(startResponse.status===404)throw Error('Upload service credential did not become available');
 const start=await startResponse.json();
 if(startResponse.status===409){
  if(start.error!=='Object already exists')throw Error('Unexpected existing-object response');
  complete=true;console.log('Existing exact-key object found; verifying its complete content before public configuration.');
 }else{
 uploadId=start.uploadId;
 if(typeof uploadId!=='string'||start.partBytes!==32*1024*1024)throw Error('Unexpected upload session');
 const file=await open(packagePath,'r'),parts=[];
 try {
  for(let offset=0,number=1;offset<bytes;offset+=start.partBytes,number++){
   const part=Buffer.alloc(Math.min(start.partBytes,bytes-offset));let filled=0;
   while(filled<part.length){const read=await file.read(part,filled,part.length-filled,offset+filled);if(!read.bytesRead)throw Error('Installer changed during upload');filled+=read.bytesRead;}
   const result=await (await request('/part/'+number+'?uploadId='+encodeURIComponent(uploadId),{method:'PUT',body:part,
    headers:{'Content-Length':String(part.length),'X-Part-SHA256':createHash('sha256').update(part).digest('hex')}})).json();
   if(result.partNumber!==number||typeof result.etag!=='string')throw Error('Unexpected part acknowledgement');parts.push(result);
   console.log(`Uploaded part ${number}/${Math.ceil(bytes/start.partBytes)}`);
  }
 }finally{await file.close();}
 await request('/finish?uploadId='+encodeURIComponent(uploadId),{method:'POST',body:JSON.stringify(parts)});complete=true;
 }
 const response=await request('/artifact',{timeoutMs:90*60*1000}),remoteHash=createHash('sha256');let received=0;
 for await(const chunk of response.body){remoteHash.update(chunk);received+=chunk.length;}
 if(received!==bytes||remoteHash.digest('hex')!==sha256)throw Error('Remote installer verification failed; keep public download disabled');
 const path='/downloads/'+key;
 await writeFile(join(releaseDirectory,'macOS-download.json'),JSON.stringify({macInstaller:'https://hungryghostaudio.com'+path,
  macArtifact:{path,key,bytes,sha256,signed:true,notarized:true}},null,2)+'\n','utf8');
 console.log('Exact signed installer uploaded and its full remote SHA-256 verified. Public download configuration is ready.');
}finally{
 if(deployed){
  if(base&&uploadId&&!complete)try{await fetch(base+'/abort?uploadId='+encodeURIComponent(uploadId),{method:'POST',headers:{Authorization:'Bearer '+token},signal:AbortSignal.timeout(30000)});}catch{}
  try{await cli(['delete','--force']);}catch{console.error('Temporary upload service cleanup failed; delete worker '+name+'. Its credential expires automatically.');}
 }
 const resolvedScratch=resolve(scratch),prefix=resolve(tmpdir())+sep;
 if(!resolvedScratch.startsWith(prefix)||!resolvedScratch.split(sep).at(-1).startsWith('hungryghost-release-upload-'))throw Error('Unexpected temporary directory');
 await rm(resolvedScratch,{recursive:true,force:true});
}
