#include "PitchEngine.h"
#include "rubberband/RubberBandLiveShifter.h"
#include <algorithm>
#include <cmath>
#include <vector>
#include <stdexcept>

namespace hungryghost::haunt {
namespace {
// Rubber Band 4.0's live interface has a fixed 512-frame input/output quantum.
constexpr int quantum=512;
using Shifter=RubberBand::RubberBandLiveShifter;
float clean(float x) { return std::isfinite(x)?std::clamp(x,-16.f,16.f):0.f; }
float pole(float milliseconds, double rate) { return milliseconds<=0?0.f:std::exp(-1.f/float(.001*milliseconds*rate)); }
}
unsigned scaleMask(int key,int scale,unsigned custom) {
    if(scale==4) return custom&4095;
    if(scale==0) return 4095;
    constexpr unsigned masks[]={4095,2741,1453,661}; // chromatic, major, minor, major pentatonic
    const unsigned mask=masks[std::clamp(scale,0,3)];
    key=(key%12+12)%12;
    return ((mask<<key)|(mask>>(12-key)))&4095;
}
float nearestNote(float note,unsigned mask,float previous) {
    if(!mask||!std::isfinite(note)) return -1;
    float best=-1, distance=1e6f;
    for(int n=0;n<128;++n) if(mask&(1u<<(n%12))) {
        const float d=std::abs(note-float(n));
        if(d<distance) { distance=d; best=float(n); }
    }
    if(previous>=0&&previous<128&&(mask&(1u<<(int(previous)%12)))
        &&std::abs(note-previous)<distance+.12f) return previous;
    return best;
}
struct PitchEngine::Impl {
    std::unique_ptr<Shifter> shift;
    Settings settings;
    Reading view;
    double rate=48000, detectorRate=12000;
    int channels=2, decimation=4, decimationPhase=0, detectorHop=60, hop=0;
    int window=288, maxLag=185, detectorWrite=0, detectorFilled=0, position=0, latency=0, dryWrite=0, analysisChannel=0;
    std::array<float,2048> detector{};
    std::array<float,1024> frame{}, difference{};
    std::array<std::array<float,quantum>,2> input{}, output{};
    std::array<std::vector<float>,2> dry;
    std::array<std::array<bool,128>,16> held{}, latched{};
    std::array<bool,16> pedal{};
    float low1=0,low2=0,low3=0,filter=0, dc=0;
    float smoothPitch=0, correction=0, target=-1, stableSeconds=0, unvoicedSeconds=0;
    float wet=1, gain=1, filteredFrequency=0;
    bool wasVoiced=false;
    void detect() {
        const int count=window+maxLag+1;
        if(detectorFilled<count) return;
        double energy=0, mean=0;
        for(int i=0;i<count;++i) { frame[i]=detector[(detectorWrite-count+i+2048)%2048]; mean+=frame[i]; }
        mean/=count;
        for(int i=0;i<count;++i) { frame[i]-=float(mean); energy+=frame[i]*frame[i]; }
        view.inputDb=float(10*std::log10(std::max(1e-12,energy/count)));
        view.voiced=false; view.frequency=0; view.confidence=0;
        if(view.inputDb<-55) return;
        double sum=0;
        difference[0]=1;
        for(int lag=1;lag<=maxLag;++lag) {
            double d=0;
            for(int i=0;i<window;++i) { const double delta=frame[i]-frame[i+lag]; d+=delta*delta; }
            sum+=d; difference[lag]=sum>1e-15?float(d*lag/sum):1.f;
        }
        const float lo[]={65,65,100,160}, hi[]={1100,350,700,1100};
        const int range=std::clamp(settings.range,0,3);
        const int first=std::max(2,int(detectorRate/hi[range]));
        const int last=std::min(maxLag-1,int(detectorRate/lo[range]));
        int chosen=-1;
        for(int lag=first;lag<=last;++lag) if(difference[lag]<.14f) {
            while(lag<last&&difference[lag+1]<difference[lag]) ++lag;
            chosen=lag; break;
        }
        if(chosen<0) return;
        const float a=difference[chosen-1], b=difference[chosen], c=difference[chosen+1];
        const float curvature=a-2*b+c;
        const float fraction=std::abs(curvature)>1e-8f?std::clamp(.5f*(a-c)/curvature,-.5f,.5f):0;
        // Refine against the waveform itself. A parabola on the normalised YIN
        // curve biases short periods, especially pure high notes, by several cents.
        double period=chosen+fraction;
        for(int iteration=0;iteration<4;++iteration) {
            const int lag=int(std::floor(period));const double t=period-lag;
            double gradient=0,hessian=0;
            for(int i=0;i<window;++i) {
                const double y0=frame[i+lag-1],y1=frame[i+lag],y2=frame[i+lag+1],y3=frame[i+lag+2];
                const double b0=.5*(y2-y0),c0=y0-2.5*y1+2*y2-.5*y3,d0=.5*(y3-y0)+1.5*(y1-y2);
                const double error=y1+t*(b0+t*(c0+t*d0))-frame[i];
                const double firstDerivative=b0+t*(2*c0+3*t*d0),secondDerivative=2*c0+6*t*d0;
                gradient+=error*firstDerivative;hessian+=firstDerivative*firstDerivative+error*secondDerivative;
            }
            if(hessian<=1e-12)break;
            period=std::clamp(period-std::clamp(gradient/hessian,-.5,.5),chosen-.75,chosen+.75);
        }
        const float frequency=float(detectorRate/period);
        if(frequency<lo[range]||frequency>hi[range]) return;
        view.frequency=frequency; view.confidence=std::clamp(1-b,0.f,1.f); view.voiced=true;
        view.note=69.f+12.f*std::log2(frequency/settings.reference);
    }
    float midiTarget(float note) const {
        float best=-1,distance=1e6f;
        for(int n=0;n<128;++n) {
            bool active=false;
            for(int c=0;c<16;++c) active|=held[c][n]||latched[c][n];
            if(active&&std::abs(note-n)<distance) { best=float(n); distance=std::abs(note-n); }
        }
        return best;
    }
    void render() {
        if(channels==2) {
            float energy[2]={};
            for(int c=0;c<2;++c)for(float x:input[c])energy[c]+=x*x;
            if(energy[1-analysisChannel]>4*energy[analysisChannel]+1e-9f)analysisChannel=1-analysisChannel;
        }
        const float dt=float(quantum/rate);
        float desired=0;
        if(view.voiced) {
            if(!wasVoiced) smoothPitch=view.note;
            smoothPitch+=(view.note-smoothPitch)*(1-std::exp(-dt/.18f));
            const float chosen=settings.midiTarget?midiTarget(view.note):nearestNote(view.note,scaleMask(settings.key,settings.scale,settings.customMask),target);
            if(chosen!=target) stableSeconds=0; else stableSeconds+=dt;
            target=chosen; unvoicedSeconds=0;
            if(target>=0) {
                const float expression=std::clamp(view.note-smoothPitch,-.65f,.65f)*settings.vibrato;
                desired=std::clamp((target-view.note+expression)*settings.amount,-24.f,24.f);
            }
        } else {
            unvoicedSeconds+=dt;
            if(unvoicedSeconds>.025f) target=-1;
        }
        wasVoiced=view.voiced;
        view.hasTarget=view.voiced&&target>=0; view.target=target>=0?target:0;
        const float longNote=std::clamp((stableSeconds-.12f)/.25f,0.f,1.f);
        const float ms=view.hasTarget?settings.retuneMs*(1+settings.humanize*longNote*5):12.f;
        const float coefficient=ms<=0?0:std::exp(-1000.f*dt/std::max(1.f,ms));
        correction=coefficient*correction+(1-coefficient)*desired;
        view.correction=correction;
        const float semitones=std::clamp(correction+settings.transpose,-24.f,24.f);
        const double pitchRatio=std::pow(2.,semitones/12.);
        shift->setPitchScale(pitchRatio);
        shift->setFormantOption(settings.preserveFormants?Shifter::OptionFormantPreserved:Shifter::OptionFormantShifted);
        // Rubber Band's explicit formant scale is relative to the pitch-scaled
        // envelope. A zero value delegates compensation to the selected option.
        shift->setFormantScale(std::abs(settings.formant)<1e-6f?0.:
            std::pow(2.,settings.formant/12.)/(settings.preserveFormants?pitchRatio:1.));
        const float* ins[]={input[0].data(),input[1].data()};
        float* outs[]={output[0].data(),output[1].data()};
        shift->shift(ins,outs);
    }
};
PitchEngine::PitchEngine():impl(std::make_unique<Impl>()) {}
PitchEngine::~PitchEngine()=default;
void PitchEngine::prepare(double sr,int channels) {
    auto& p=*impl;
    p.rate=std::isfinite(sr)?std::clamp(sr,8000.,384000.):48000.; p.channels=std::clamp(channels,1,2);
    p.decimation=std::max(1,int(std::round(p.rate/12000.))); p.detectorRate=p.rate/p.decimation;
    p.window=int(std::round(.024*p.detectorRate)); p.maxLag=int(std::ceil(p.detectorRate/65))+1;
    p.detectorHop=std::max(1,int(std::round(.005*p.detectorRate)));
    p.filter=1-std::exp(-float(2*3.141592653589793*2200/p.rate));
    p.shift=std::make_unique<Shifter>(size_t(p.rate),size_t(p.channels),Shifter::OptionWindowShort|Shifter::OptionChannelsTogether);
    if(p.shift->getBlockSize()!=quantum)throw std::runtime_error("Unexpected live pitch-shifter block size");
    p.latency=int(p.shift->getStartDelay())+quantum;
    for(auto& channel:p.dry) channel.assign(p.latency,0);
    reset();
}
void PitchEngine::reset() {
    auto& p=*impl;if(!p.shift){allNotesOff();return;}
    p.shift->setPitchScale(1.);p.shift->reset();p.detector.fill(0);p.input={};p.output={};
    // Anchor startup to unity before automation can alter the pitch ratio.
    // This also performs any first-call setup outside the audio callback.
    const float* ins[]={p.input[0].data(),p.input[1].data()};
    float* outs[]={p.output[0].data(),p.output[1].data()};
    p.shift->shift(ins,outs);p.output={};
    for(auto& d:p.dry) std::fill(d.begin(),d.end(),0);
    p.view={};p.detectorWrite=p.detectorFilled=p.position=p.dryWrite=p.hop=p.decimationPhase=0;
    p.low1=p.low2=p.low3=p.dc=p.correction=p.smoothPitch=p.stableSeconds=p.unvoicedSeconds=p.filteredFrequency=0;
    const bool neutral=p.settings.amount==0&&p.settings.transpose==0&&p.settings.formant==0;
    p.target=-1;p.analysisChannel=0;p.wasVoiced=false;p.wet=p.settings.bypass||neutral?0:p.settings.mix;p.gain=p.settings.bypass?1.f:std::pow(10.f,p.settings.outputDb/20);
    allNotesOff();
}
void PitchEngine::setSettings(const Settings& supplied) {
    auto s=supplied;
    const auto bounded=[](float value,float lo,float hi,float fallback){return std::isfinite(value)?std::clamp(value,lo,hi):fallback;};
    s.retuneMs=bounded(s.retuneMs,0,250,35);s.amount=bounded(s.amount,0,1,1);
    s.humanize=bounded(s.humanize,0,1,.35f);s.vibrato=bounded(s.vibrato,0,1,.5f);
    s.formant=bounded(s.formant,-12,12,0);s.transpose=bounded(s.transpose,-12,12,0);
    s.reference=bounded(s.reference,420,460,440);s.mix=bounded(s.mix,0,1,1);s.outputDb=bounded(s.outputDb,-24,12,0);
    s.key=std::clamp(s.key,0,11);s.scale=std::clamp(s.scale,0,4);s.range=std::clamp(s.range,0,3);s.customMask&=4095;
    impl->settings=s;
}
void PitchEngine::midiNote(int c,int note,bool down) {
    if(c<1||c>16||note<0||note>127) return;
    auto& p=*impl;--c;p.held[c][note]=down;
    p.latched[c][note]=!down&&p.pedal[c];
}
void PitchEngine::sustain(int c,bool down) {
    if(c<1||c>16)return;auto& p=*impl;--c;p.pedal[c]=down;if(!down)p.latched[c].fill(false);
}
void PitchEngine::allNotesOff(int c) {
    auto& p=*impl;
    if(c>=1&&c<=16) {p.held[c-1].fill(false);p.latched[c-1].fill(false);p.pedal[c-1]=false;}
    else {p.held={};p.latched={};p.pedal={};}
}
void PitchEngine::process(float* const* buffers,int count,int samples) {
    auto& p=*impl;if(p.latency<=0||count<1)return;
    count=std::min(count,p.channels);
    const float smooth=pole(8,p.rate), outputGain=p.settings.bypass?1.f:std::pow(10.f,p.settings.outputDb/20);
    const bool neutral=p.settings.amount==0&&p.settings.transpose==0&&p.settings.formant==0;
    const float desiredWet=p.settings.bypass||neutral?0.f:p.settings.mix;
    for(int i=0;i<samples;++i) {
        const float strongest=clean(buffers[std::min(count-1,p.analysisChannel)][i]);
        // Track one stable channel instead of summing phase-opposed stereo vocals.
        for(int c=0;c<count;++c) {
            const float sample=clean(buffers[c][i]);p.input[c][p.position]=sample;
        }
        p.low1+=p.filter*(strongest-p.low1);p.low2+=p.filter*(p.low1-p.low2);p.low3+=p.filter*(p.low2-p.low3);
        if(++p.decimationPhase>=p.decimation) {
            p.decimationPhase=0;p.detector[p.detectorWrite]=p.low3;p.detectorWrite=(p.detectorWrite+1)%2048;
            p.detectorFilled=std::min(2048,p.detectorFilled+1);
            if(++p.hop>=p.detectorHop){p.hop=0;p.detect();}
        }
        p.wet=smooth*p.wet+(1-smooth)*desiredWet;p.gain=smooth*p.gain+(1-smooth)*outputGain;
        for(int c=0;c<count;++c) {
            const float delayed=p.dry[c][p.dryWrite];p.dry[c][p.dryWrite]=p.input[c][p.position];
            const float shifted=clean(p.output[c][p.position]);
            buffers[c][i]=clean((delayed+p.wet*(shifted-delayed))*p.gain);
        }
        p.dryWrite=(p.dryWrite+1)%p.latency;++p.view.samples;
        if(++p.position==quantum){p.render();p.position=0;}
    }
}
int PitchEngine::latencySamples() const { return impl->latency; }
Reading PitchEngine::reading() const { return impl->view; }
}
