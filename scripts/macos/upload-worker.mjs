// Temporary, isolated upload service. Never attach it to a storefront route.
export const PART_BYTES=32*1024*1024;
const reply=(value,status=200)=>new Response(JSON.stringify(value),{status,headers:{'Content-Type':'application/json','Cache-Control':'no-store'}});
export default {async fetch(request,env){
 const size=Number(env.ARTIFACT_BYTES), expires=Number(env.EXPIRES_AT);
 if(!env.UPLOAD_TOKEN || request.headers.get('Authorization')!=='Bearer '+env.UPLOAD_TOKEN
   || !Number.isSafeInteger(expires) || Date.now()>=expires) return reply({error:'Unavailable'},404);
 if(!env.RELEASES || !Number.isSafeInteger(size) || size<=0 || !/^[a-f0-9]{64}$/.test(env.ARTIFACT_SHA256||'')
   || !/^macos\/[A-Za-z0-9._-]+\/[a-f0-9]{64}\/HungryGhostSuite-[A-Za-z0-9._-]+\.pkg$/.test(env.ARTIFACT_KEY||'')
   || env.ARTIFACT_KEY.split('/')[2]!==env.ARTIFACT_SHA256) return reply({error:'Invalid upload configuration'},503);
 const url=new URL(request.url), count=Math.ceil(size/PART_BYTES);
 try {
  if(url.pathname==='/start' && request.method==='POST') {
   if(await env.RELEASES.head(env.ARTIFACT_KEY))return reply({error:'Object already exists'},409);
   const upload=await env.RELEASES.createMultipartUpload(env.ARTIFACT_KEY,{customMetadata:{sha256:env.ARTIFACT_SHA256},httpMetadata:{contentType:'application/octet-stream'}});
   return reply({uploadId:upload.uploadId,partBytes:PART_BYTES});
  }
  if(url.pathname==='/artifact' && request.method==='GET') {
   const object=await env.RELEASES.get(env.ARTIFACT_KEY);
   if(!object?.body || object.size!==size || object.customMetadata?.sha256!==env.ARTIFACT_SHA256)return reply({error:'Object unavailable'},404);
   return new Response(object.body,{headers:{'Content-Length':String(size),'Cache-Control':'no-store'}});
  }
  const uploadId=url.searchParams.get('uploadId');
  if(!uploadId || uploadId.length>1000)return reply({error:'Upload ID required'},400);
  const upload=env.RELEASES.resumeMultipartUpload(env.ARTIFACT_KEY,uploadId);
  if(url.pathname==='/abort' && request.method==='POST'){await upload.abort();return reply({aborted:true});}
  const match=/^\/part\/([1-9][0-9]*)$/.exec(url.pathname);
  if(match && request.method==='PUT') {
   const number=Number(match[1]), length=number===count?size-PART_BYTES*(count-1):PART_BYTES;
   if(!Number.isSafeInteger(number)||number>count||Number(request.headers.get('Content-Length'))!==length)return reply({error:'Invalid part size'},400);
   const bytes=await request.arrayBuffer();
   if(bytes.byteLength!==length)return reply({error:'Invalid part body'},400);
   const hash=Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',bytes)),b=>b.toString(16).padStart(2,'0')).join('');
   if(hash!==request.headers.get('X-Part-SHA256'))return reply({error:'Part checksum mismatch'},400);
   return reply(await upload.uploadPart(number,bytes));
  }
  if(url.pathname==='/finish' && request.method==='POST') {
   const body=await request.text();if(body.length>65536)return reply({error:'Parts list too large'},400);
   const parts=JSON.parse(body);
   if(!Array.isArray(parts)||parts.length!==count||parts.some((p,i)=>p.partNumber!==i+1||typeof p.etag!=='string'||p.etag.length>200))return reply({error:'Invalid parts list'},400);
   await upload.complete(parts);
   const object=await env.RELEASES.head(env.ARTIFACT_KEY);
   if(!object || object.size!==size || object.customMetadata?.sha256!==env.ARTIFACT_SHA256)return reply({error:'Completed object mismatch'},503);
   return reply({bytes:object.size,sha256:env.ARTIFACT_SHA256});
  }
  return reply({error:'Not found'},404);
 }catch{return reply({error:'Upload operation failed'},503);}
}};
