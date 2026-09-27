import {readFileSync,writeFileSync} from 'node:fs';import {signEntitlement} from '../site/src/worker.mjs';
const [keyPath,out]=process.argv.slice(2);if(!keyPath||!out)throw Error('Supply the external private-key path and a fixture output path.');
const key=readFileSync(keyPath,'utf8'),now=Math.floor(Date.now()/1000),device='a'.repeat(64);
const base={v:1,device,products:['feral','rift'],issued:now-60,expires:now+86400,activationId:'test-activation',licenceId:'test-licence'};
const valid=await signEntitlement(base,key),expired=await signEntitlement({...base,issued:now-172800,expires:now-86400},key),future=await signEntitlement({...base,issued:now+3600,expires:now+86400},key),version=await signEntitlement({...base,v:2},key);
writeFileSync(out,JSON.stringify({now,device,valid,expired,future,version},null,2));console.log('Signed test fixtures written. No private material included.');
