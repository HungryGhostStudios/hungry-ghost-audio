#include "Haunt/PitchEngine.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <vector>
namespace allocationCheck { thread_local bool enabled=false; thread_local size_t count=0; }
void* operator new(std::size_t bytes){if(allocationCheck::enabled)++allocationCheck::count;if(void* p=std::malloc(bytes?bytes:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t bytes){return ::operator new(bytes);}
void operator delete(void* p)noexcept{std::free(p);}
void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}
void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
using namespace hungryghost::haunt;
namespace {
constexpr double pi=3.141592653589793;
void require(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
std::vector<float> tone(double sr,double hz,double seconds,bool harmonics=false){std::vector<float> x(size_t(sr*seconds));for(size_t i=0;i<x.size();++i){const double p=2*pi*hz*i/sr;x[i]=float(.24*std::sin(p)+(harmonics?.35*std::sin(2*p)+.18*std::sin(3*p):0));}return x;}
void process(PitchEngine& engine,std::vector<float>& data,int block=257){for(int at=0;at<int(data.size());at+=block){float* p[]={data.data()+at};engine.process(p,1,std::min(block,int(data.size())-at));}}
double measuredHz(const std::vector<float>& y,double sr){double first=0,last=0;int crossings=0;for(size_t i=y.size()/2+1;i<y.size();++i)if(y[i-1]<=0&&y[i]>0){const double cross=i-1-y[i-1]/(y[i]-y[i-1]);if(crossings==0)first=cross;last=cross;++crossings;}require(crossings>10,"Insufficient output crossings");return (crossings-1)*sr/(last-first);}

void harmonicIntegrity() {
    // A single sine can be correctly tuned while vocal harmonics are smeared.
    // Keep this regression independent of the shifter's FFT and pitch display.
    constexpr int rate=48000,n=rate*4;
    const double inputFrequency=220*std::pow(2.,.32/12);
    std::vector<float> vocal(n,0);
    for(int harmonic=1;harmonic<=60;++harmonic) {
        const double frequency=harmonic*inputFrequency;
        const auto bell=[&](double centre,double width){return std::exp(-.5*std::pow((frequency-centre)/width,2));};
        const double amplitude=(.06+bell(650,95)+.7*bell(1200,140)+.4*bell(2600,200))/harmonic;
        for(int i=0;i<n;++i)vocal[i]+=float(amplitude*std::cos(2*pi*frequency*i/rate));
    }
    float peak=0;for(float sample:vocal)peak=std::max(peak,std::abs(sample));for(float& sample:vocal)sample*=.5f/peak;
    for(bool preserve:{false,true}) {
        PitchEngine engine;Settings settings;settings.amount=0;settings.transpose=-.32f;settings.preserveFormants=preserve;
        engine.setSettings(settings);engine.prepare(rate,1);auto output=vocal;process(engine,output);
        // Two seconds contain an integer number of 220-Hz cycles. Projection
        // onto all expected sine/cosine harmonics allows arbitrary phase/envelope.
        constexpr int count=rate*2;
        double total=0,harmonicEnergy=0;for(int i=n-count;i<n;++i)total+=double(output[i])*output[i];
        for(int harmonic=1;harmonic<=60;++harmonic) {
            const double angle=2*pi*220*harmonic/rate,cosStep=std::cos(angle),sinStep=std::sin(angle);
            double cosine=1,sine=0,real=0,imag=0;
            for(int i=n-count;i<n;++i){real+=output[i]*cosine;imag+=output[i]*sine;const double next=cosine*cosStep-sine*sinStep;sine=sine*cosStep+cosine*sinStep;cosine=next;}
            harmonicEnergy+=2*(real*real+imag*imag)/count;
        }
        const double residualDb=10*std::log10(std::max(1e-15,(total-harmonicEnergy)/total));
        std::cout<<"Vocal-harmonic residual / formants "<<preserve<<": "<<residualDb<<" dB\n";
        require(residualDb<-35,"Pitch shifter introduced excessive non-harmonic vocal artifacts");
    }
}
}
int main(int argc,char** argv){try {
    harmonicIntegrity();
    if(argc>1&&std::string(argv[1])=="--quality-only")return 0;
    require(scaleMask(0,1)==2741&&scaleMask(0,2)==1453,"Incorrect scale notes");
    require(nearestNote(61.2f,scaleMask(0,1))==62,"Scale target selection");
    require(nearestNote(60.f,0)<0,"Empty mask must not invent a target");
    for(double sr:{44100.,48000.,96000.}) {
        for(double hz:{73.42,110.,220.,440.,880.}) {
            PitchEngine e;e.prepare(sr,1);Settings s;s.vibrato=0;s.humanize=0;s.retuneMs=0;e.setSettings(s);
            auto x=tone(sr,hz,.7,true);process(e,x);const auto r=e.reading();
            const double cents=1200*std::log2(r.frequency/hz);
            std::cout<<"tracking "<<sr<<" Hz / "<<hz<<" Hz: "<<cents<<" cents, confidence "<<r.confidence<<"\n";
            require(r.voiced&&std::abs(cents)<2,"Tracking accuracy or harmonic octave error");
        }
        PitchEngine e;Settings s;s.retuneMs=0;s.vibrato=s.humanize=0;s.preserveFormants=false;e.setSettings(s);e.prepare(sr,1);
        auto x=tone(sr,440*std::pow(2.,.32/12),1.4);process(e,x);
        const auto hz=measuredHz(x,sr);std::cout<<"corrected output "<<hz<<" Hz; detected "<<e.reading().frequency<<" Hz; correction "<<e.reading().correction<<" st; reported latency "<<1000*e.latencySamples()/sr<<" ms\n";
        require(std::abs(1200*std::log2(hz/440))<2,"Actual audio is not tuned to A4");
        for(float transpose:{-.32f,-1.f,-4.f,0.f,1.f}) {
            PitchEngine fixed;Settings fs;fs.amount=0;fs.transpose=transpose;fs.preserveFormants=false;fixed.setSettings(fs);fixed.prepare(sr,1);
            auto steady=tone(sr,440*std::pow(2.,.32/12),3.);process(fixed,steady);
            const auto expected=440*std::pow(2.,(.32+transpose)/12);
            require(std::abs(1200*std::log2(measuredHz(steady,sr)/expected))<3,"Fixed transposition pitch error");
        }
        s.bypass=true;s.outputDb=-12;e.setSettings(s);e.reset();auto impulse=std::vector<float>(e.latencySamples()+700,0);impulse[0]=.6f;process(e,impulse);
        for(int i=0;i<int(impulse.size());++i)require(std::abs(impulse[i]-(i==e.latencySamples()?.6f:0.f))<1e-6f,"Bypass delay is not aligned with reported latency");
    }
    for(double targetHz:{73.41619198,110.,220.,440.,880.,1046.502261})for(double detune:{-.32,.32})for(bool preserve:{false,true}) {
        PitchEngine e;Settings s;s.retuneMs=0;s.vibrato=s.humanize=0;s.preserveFormants=preserve;e.setSettings(s);e.prepare(48000,1);
        auto audio=tone(48000,targetHz*std::pow(2.,detune/12),1.5);process(e,audio);
        const auto cents=1200*std::log2(measuredHz(audio,48000)/targetHz);
        std::cout<<"output grid "<<targetHz<<" Hz / "<<detune<<" st / formants "<<preserve<<": "<<cents<<" cents\n";
        require(std::abs(cents)<3,"Output correction outside 3 cents on tuning grid");
    }
    Settings s;s.vibrato=s.humanize=0;s.retuneMs=0;s.midiTarget=true;
    PitchEngine midi;midi.prepare(48000,1);midi.setSettings(s);midi.midiNote(1,72,true);
    auto x=tone(48000,440,.7);process(midi,x);require(midi.reading().target==72,"MIDI targeting did not select exact held octave");
    midi.sustain(1,true);midi.midiNote(1,72,false);x=tone(48000,440,.1);process(midi,x);require(midi.reading().hasTarget,"Sustain did not retain target");
    midi.sustain(1,false);x=tone(48000,440,.1);process(midi,x);require(!midi.reading().hasTarget,"Released MIDI target remains stuck");
    midi.midiNote(2,67,true);midi.midiNote(1,67,false);x=tone(48000,440,.1);process(midi,x);require(midi.reading().target==67,"One MIDI channel released another channel's target");
    midi.allNotesOff();midi.reset();x.assign(10000,0);x[55]=std::numeric_limits<float>::quiet_NaN();x[80]=std::numeric_limits<float>::infinity();process(midi,x);for(float v:x)require(std::isfinite(v),"Non-finite input poisoned output");require(!midi.reading().voiced,"Silence registered as a note");
    std::mt19937 random(2130);std::uniform_real_distribution<float> noise(-.2f,.2f);for(float& v:x)v=noise(random);process(midi,x);require(!midi.reading().voiced,"Unvoiced noise registered as a note");
    PitchEngine one,two;one.prepare(48000,1);two.prepare(48000,1);s.midiTarget=false;one.setSettings(s);two.setSettings(s);
    auto a=tone(48000,274,.7,true),b=a;process(one,a,1);process(two,b,4097);for(size_t i=0;i<a.size();++i)require(std::abs(a[i]-b[i])<1e-6,"Block size changed the pitch engine output");
    PitchEngine stereo;stereo.prepare(48000,2);auto left=tone(48000,220,.6,true),right=left;for(float& v:right)v=-v;
    float* pair[]={left.data(),right.data()};stereo.process(pair,2,int(left.size()));require(stereo.reading().voiced&&std::abs(stereo.reading().frequency-220)<1,"Phase-opposed stereo lost pitch detection");
    PitchEngine unity;Settings neutral;neutral.amount=0;neutral.preserveFormants=false;unity.setSettings(neutral);unity.prepare(48000,1);
    auto original=tone(48000,257,1,true),aligned=original;process(unity,aligned);
    double squareError=0,signalEnergy=0;for(size_t i=size_t(unity.latencySamples())+4800;i<aligned.size();++i){const double expected=original[i-unity.latencySamples()];squareError+=std::pow(aligned[i]-expected,2);signalEnergy+=expected*expected;}
    const double nullDb=10*std::log10(squareError/signalEnergy);std::cout<<"Neutral wet path null: "<<nullDb<<" dB\n";require(nullDb<-70,"Neutral wet signal does not align with reported latency");
    PitchEngine realtime;realtime.prepare(48000,2);auto rtLeft=tone(48000,317,6,true),rtRight=rtLeft;
    Settings rtSettings;rtSettings.formant=2;realtime.setSettings(rtSettings);
    const auto start=std::chrono::steady_clock::now();allocationCheck::enabled=true;
    for(int at=0;at<int(rtLeft.size());at+=128){float* ptr[]={rtLeft.data()+at,rtRight.data()+at};realtime.process(ptr,2,std::min(128,int(rtLeft.size())-at));}
    allocationCheck::enabled=false;
    const double elapsed=std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count();
    std::cout<<"6 seconds stereo/formants processed in "<<elapsed<<" seconds; C++ process allocations: "<<allocationCheck::count<<"\n";
    require(allocationCheck::count==0,"Audio processing allocated C++ heap memory");
    rtSettings.reference=std::numeric_limits<float>::quiet_NaN();rtSettings.outputDb=std::numeric_limits<float>::infinity();realtime.setSettings(rtSettings);
    float* rtPair[]={rtLeft.data(),rtRight.data()};realtime.process(rtPair,2,512);for(int i=0;i<512;++i)require(std::isfinite(rtLeft[i]),"Invalid settings poisoned audio");
    std::cout<<"PASS: pitch tracking, audio correction, MIDI/sustain, latency, silence/noise, non-finite recovery, stereo and block invariance\n";
    return 0;
}catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
