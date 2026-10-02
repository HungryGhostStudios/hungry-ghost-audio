export const analyticsEvents=new Set(['page_view','listen_view','demo_play','demo_toggle','trial_download','checkout_opened']);
const text=(value,max=120)=>typeof value==='string'?value.trim().slice(0,max):'';
const campaignKeys=['utm_source','utm_medium','utm_campaign','utm_content'];

function referrerHost(request){
 try{return new URL(request.headers.get('Referer')||'').hostname.toLowerCase().slice(0,120);}catch{return '';}
}

export function analyticsPoint(request,event,details={}){
 if(!analyticsEvents.has(event))throw Error('Unknown analytics event');
 const url=new URL(request.url),campaign={};
 for(const key of campaignKeys)campaign[key]=text(details[key]||url.searchParams.get(key));
 return {
  blobs:[event,text(details.path||url.pathname,180),text(details.product),text(details.label),campaign.utm_source,campaign.utm_medium,campaign.utm_campaign,campaign.utm_content,referrerHost(request),text(request.cf?.country,2)],
  doubles:[1],
  indexes:[event]
 };
}

export function recordAnalytics(env,request,event,details={}){
 if(!env.ANALYTICS?.writeDataPoint)return false;
 try{env.ANALYTICS.writeDataPoint(analyticsPoint(request,event,details));return true;}catch{return false;}
}
