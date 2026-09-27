// Private material goes only into the explicitly supplied directory, never the repository.
import {generateKeyPairSync,createPrivateKey,createPublicKey} from 'node:crypto';
import {mkdirSync,existsSync,readFileSync,writeFileSync} from 'node:fs';
import {resolve,join} from 'node:path';
import {fileURLToPath} from 'node:url';
const secretDirectory=process.argv[2];if(!secretDirectory)throw Error('Supply an external private-key directory.');
const root=resolve(fileURLToPath(new URL('..',import.meta.url))),directory=resolve(secretDirectory);
if(directory.startsWith(root+'\\')||directory.startsWith(root+'/'))throw Error('Private keys cannot be stored in the source repository.');
mkdirSync(directory,{recursive:true});const keyFile=join(directory,'licence-signing.pem');
let privateKey;if(existsSync(keyFile))privateKey=createPrivateKey(readFileSync(keyFile));else{privateKey=generateKeyPairSync('rsa',{modulusLength:3072,publicExponent:65537}).privateKey;writeFileSync(keyFile,privateKey.export({type:'pkcs8',format:'pem'}),{mode:0o600});}
const publicKey=createPublicKey(privateKey),jwk=publicKey.export({format:'jwk'});
const e=Buffer.from(jwk.e,'base64url').toString('hex'),n=Buffer.from(jwk.n,'base64url').toString('hex');
writeFileSync(join(root,'Source/Licensing/PublicKey.h'),`#pragma once\nnamespace hungryghost { inline constexpr const char* licencePublicKey = "${e},${n}"; inline constexpr int licenceSignatureBytes = 384; }\n`);
writeFileSync(join(root,'site/public/licence-public-key.pem'),publicKey.export({type:'spki',format:'pem'}));
console.log('3072-bit public verification key generated. Private key retained outside the repository.');
