// Versioned installers live in private object storage. Only a signed,
// notarized artifact explicitly included in the release configuration is public.
export async function releaseDownload(request, env, config) {
 const url = new URL(request.url);
 // History is an explicit trusted allowlist, not a prefix or bucket lookup.
 // Keep exactly one record per immutable path, including the current artifact.
 const history = Array.isArray(config.downloads?.macArtifacts) ? config.downloads.macArtifacts : [];
 const matches = [config.downloads?.macArtifact, ...history].filter(item => item?.path === url.pathname);
 const artifact = matches.length === 1 ? matches[0] : null;
 const unavailable = () => new Response('Download not found', {status:404});
 if (!artifact || artifact.signed !== true || artifact.notarized !== true || !env.RELEASES
     || artifact.path !== url.pathname || !/^[a-f0-9]{64}$/.test(artifact.sha256 || '')
     || !Number.isSafeInteger(artifact.bytes) || artifact.bytes <= 0
     || !/^macos\/[A-Za-z0-9._-]+\/[a-f0-9]{64}\/HungryGhostSuite-[A-Za-z0-9._-]+\.pkg$/.test(artifact.key || '')
     || artifact.key.split('/')[2] !== artifact.sha256
     || artifact.path !== '/downloads/' + artifact.key) return unavailable();
 if (!['GET','HEAD'].includes(request.method)) return new Response(null,{status:405,headers:{Allow:'GET, HEAD'}});
 const head = await env.RELEASES.head(artifact.key);
 if (!head || head.size !== artifact.bytes || head.customMetadata?.sha256 !== artifact.sha256)
  return new Response('Download temporarily unavailable',{status:503});
 const etag = '"'+artifact.sha256+'"', filename = artifact.key.split('/').at(-1);
 const headers = new Headers({'Content-Type':'application/octet-stream',
  'Content-Disposition':`attachment; filename="${filename}"`, 'Accept-Ranges':'bytes',
  'Cache-Control':'public, max-age=31536000, immutable', ETag:etag, 'X-Content-Type-Options':'nosniff'});
 if (request.headers.get('If-None-Match') === etag) return new Response(null,{status:304,headers});
 let offset=0, length=head.size, status=200;
 const range=request.headers.get('Range'), ifRange=request.headers.get('If-Range');
 if (range && (!ifRange || ifRange === etag)) {
  const match=/^bytes=(\d*)-(\d*)$/.exec(range);
  if (!match || !(match[1] || match[2])) return new Response(null,{status:416,headers:{'Content-Range':`bytes */${head.size}`}});
  if (!match[1]) {
   const suffix=Number(match[2]);
   if (!Number.isSafeInteger(suffix) || suffix<=0) return new Response(null,{status:416,headers:{'Content-Range':`bytes */${head.size}`}});
   offset=Math.max(0,head.size-suffix); length=head.size-offset;
  } else {
   offset=Number(match[1]); const requestedEnd=match[2]?Number(match[2]):head.size-1;
   const end=Math.min(requestedEnd,head.size-1);
   if (!Number.isSafeInteger(offset) || !Number.isSafeInteger(requestedEnd) || offset>=head.size || end<offset)
    return new Response(null,{status:416,headers:{'Content-Range':`bytes */${head.size}`}});
   length=end-offset+1;
  }
  status=206; headers.set('Content-Range',`bytes ${offset}-${offset+length-1}/${head.size}`);
 }
 headers.set('Content-Length',String(length));
 if (request.method === 'HEAD') return new Response(null,{status,headers});
 const object=await env.RELEASES.get(artifact.key,status===206?{range:{offset,length}}:undefined);
 if (!object?.body || object.size!==head.size || object.customMetadata?.sha256!==artifact.sha256)
  return new Response('Download temporarily unavailable',{status:503});
 return new Response(object.body,{status,headers});
}
