#pragma once
#include "../Catalogue.h"
#include <juce_core/juce_core.h>
#include <array>

namespace hungryghost {
struct ProductPreset {
  const char* name;
  std::array<float,6> values;
  float mix=1.f;
};
inline std::array<ProductPreset,3> presetsFor(const Product& p) {
  std::array<float,6> base{};
  for(int i=0;i<6;++i) base[i]=p.controls[i].initial;
  ProductPreset a{"Factory",base},b{"",base},c{"",base};
  if(p.kind>=Kind::Delay&&p.kind<=Kind::Comb)a.mix=.3f;
  else if(p.kind==Kind::ParallelCompressor||p.kind==Kind::Chorus||p.kind==Kind::Flanger||p.kind==Kind::Phaser)a.mix=.5f;
  switch(p.kind) {
  case Kind::Compressor: case Kind::FastCompressor: case Kind::RmsCompressor:
  case Kind::BusCompressor: case Kind::ParallelCompressor:
    b.name="Preserve the attack"; b.values[0]=-20; b.values[1]=3; b.values[2]=25; b.values[3]=160;
    c.name="Hold the peaks"; c.values[0]=-24; c.values[1]=8; c.values[2]=1; c.values[3]=80;
    if(p.kind==Kind::ParallelCompressor) {b.mix=.5f;c.mix=.35f;}
    break;
  case Kind::DeEsser: b.name="Vocal sibilance"; b.values={-24,6500,1.5f,120,8,2}; c.name="Bright cymbals"; c.values={-20,9500,1,80,5,1}; break;
  case Kind::Gate: b.name="Drum spill"; b.values={-30,-50,1,90,4,30}; c.name="Quiet pauses"; c.values={-50,-80,5,240,6,100}; break;
  case Kind::Expander: b.name="Gentle cleanup"; b.values={-45,1.5f,5,220,12,0}; c.name="Tight drums"; c.values={-30,4,1,80,36,0}; break;
  case Kind::Transient: b.name="Attack forward"; b.values={55,-25,5,120,1,0}; c.name="Soften the hit"; c.values={-45,20,12,220,1,0}; break;
  case Kind::Sustain: b.name="Longer body"; b.values={50,85,8,400,1,0}; c.name="Tighten the tail"; c.values={-55,90,5,120,1,0}; break;
  case Kind::Limiter: b.name="Catch occasional peaks"; b.values[0]=-1;b.values[1]=0;b.values[2]=100; c.name="Dense drum bus"; c.values[0]=-1;c.values[1]=8;c.values[2]=65;break;
  case Kind::Clipper: b.name="Round the peaks";b.values[0]=-2;b.values[1]=4;b.values[2]=80;c.name="Hard drum edge";c.values[0]=-1;c.values[1]=7;c.values[2]=10;break;
  case Kind::Leveller: b.name="Steady vocal";b.values={-20,6,500,1800,-55,0};c.name="Slow instrument ride";c.values={-22,4,1000,3000,-60,0};break;
  case Kind::Ducker: b.name="Vocal over music";b.values={-30,4,8,280,10,0};c.name="Kick clearance";c.values={-24,10,1,100,18,0};break;
  case Kind::ParametricEQ: b.name="Warm foundation";b.values={110,3,850,-2,7000,-1};c.name="Clear and open";c.values={180,-2,2500,2,10000,3};break;
  case Kind::TiltEQ: b.name="Warmer";b.values[0]=800;b.values[1]=-3;c.name="Brighter";c.values[0]=1500;c.values[1]=3;break;
  case Kind::LowShelf: b.name="Low-end weight";b.values[0]=100;b.values[1]=4;b.values[2]=.707f;c.name="Less rumble";c.values[0]=180;c.values[1]=-5;c.values[2]=.707f;break;
  case Kind::HighShelf: b.name="Air above the mix";b.values[0]=10000;b.values[1]=4;b.values[2]=.707f;c.name="Softer top";c.values[0]=6000;c.values[1]=-5;c.values[2]=.707f;break;
  case Kind::Notch: b.name="Mains hum 50 Hz";b.values[0]=50;b.values[1]=10;c.name="Mains hum 60 Hz";c.values[0]=60;c.values[1]=10;break;
  case Kind::BandPass: b.name="Telephone band";b.values[0]=1400;b.values[1]=.7f;c.name="Focused midrange";c.values[0]=900;c.values[1]=1.5f;break;
  case Kind::Cuts: b.name="Vocal boundaries";b.values={90,16000,.707f,0,0,0};c.name="Small speaker";c.values={350,4200,.707f,0,0,0};break;
  case Kind::MidSideEQ: b.name="Wider presence";b.values={4500,0,3,.707f,0,0};c.name="Centred low mids";c.values={240,1,-4,.707f,0,0};break;
  case Kind::SoftSaturation: b.name="Warm instrument";b.values[0]=8;b.values[1]=9000;b.mix=.6f;c.name="Driven texture";c.values[0]=22;c.values[1]=6000;c.mix=.8f;break;
  case Kind::AsymmetricSaturation: b.name="Even harmonic colour";b.values={12,.18f,10000,0,0,0};b.mix=.6f;c.name="Uneven edge";c.values={24,.35f,7000,0,0,0};c.mix=.8f;break;
  case Kind::TubeSaturation: b.name="Soft glow";b.values={10,25,10000,0,0,0};b.mix=.65f;c.name="Dense heat";c.values={24,75,6500,0,0,0};c.mix=.8f;break;
  case Kind::Wavefolder: b.name="Folded synth";b.values={14,0,10000,0,0,0};b.mix=.6f;c.name="Asymmetric folds";c.values={26,.25f,8000,0,0,0};c.mix=.7f;break;
  case Kind::Rectifier: b.name="Octave texture";b.values={65,.05f,10000,0,0,0};b.mix=.5f;c.name="Broken signal";c.values={100,.3f,5000,0,0,0};c.mix=.8f;break;
  case Kind::BitCrusher: b.name="Twelve-bit colour";b.values={12,15,12000,0,0,0};c.name="Four-bit fracture";c.values={4,0,7000,0,0,0};c.mix=.7f;break;
  case Kind::RateReducer: b.name="Early digital";b.values[0]=12000;b.values[1]=10000;c.name="Aliased texture";c.values[0]=3500;c.values[1]=8000;c.mix=.7f;break;
  case Kind::Delay: b.name="Quarter-note at 120";b.values={500,35,8000,0,0,0};b.mix=.25f;c.name="Dotted eighth at 120";c.values={375,48,5000,15,0,0};c.mix=.35f;break;
  case Kind::TapeDelay: b.name="Warm repeat";b.values={420,35,5000,20,8,0};b.mix=.3f;c.name="Worn long echo";c.values={650,60,3500,45,16,0};c.mix=.4f;break;
  case Kind::PingPong: b.name="Wide eighth at 120";b.values={250,40,8000,0,0,0};b.mix=.3f;c.name="Floating stereo";c.values={550,60,5000,20,0,0};c.mix=.4f;break;
  case Kind::SlapDelay: b.name="Close vocal slap";b.values={90,10,6000,0,0,0};b.mix=.2f;c.name="Rockabilly repeat";c.values={140,22,4500,0,0,0};c.mix=.3f;break;
  case Kind::DubDelay: b.name="Filtered dub";b.values={500,60,3000,20,0,0};b.mix=.35f;c.name="Long feedback";c.values={750,78,1800,40,0,0};c.mix=.4f;break;
  case Kind::Comb: b.name="Metal resonance";b.values={8,65,10000,0,0,0};b.mix=.5f;c.name="Short hollow body";c.values={22,45,6000,0,0,0};c.mix=.4f;break;
  case Kind::Chorus: b.name="Wide ensemble";b.values={.35f,55,22,12,0,0};b.mix=.55f;c.name="Fast shimmer";c.values={2.2f,40,14,8,0,0};c.mix=.45f;break;
  case Kind::Flanger: b.name="Slow sweep";b.values={.15f,70,3,55,0,0};b.mix=.5f;c.name="Jet motion";c.values={.8f,85,1,75,0,0};c.mix=.6f;break;
  case Kind::Phaser: b.name="Slow phase";b.values={.2f,65,800,45,0,0};b.mix=.6f;c.name="Rhythmic phase";c.values={1.2f,80,1200,60,0,0};c.mix=.65f;break;
  case Kind::Tremolo: b.name="Soft pulse";b.values={3,45,0,0,0,0};c.name="Chopped pulse";c.values={6,90,90,0,0,0};break;
  case Kind::AutoPan: b.name="Slow travel";b.values={.3f,70,0,0,0,0};c.name="Fast alternation";c.values={2.5f,95,75,0,0,0};break;
  case Kind::RingMod: b.name="Low metallic movement";b.values={38,65,0,0,0,0};b.mix=.5f;c.name="Bell sidebands";c.values={440,100,0,0,0,0};c.mix=.7f;break;
  case Kind::Vibrato: b.name="Gentle drift";b.values={4,20,10,0,0,0};c.name="Unsteady tape";c.values={.7f,60,18,0,0,0};break;
  case Kind::Width: b.name="Subtle width";b.values={115,160,0,0,0,0};c.name="Wide with centred bass";c.values={165,220,0,0,0,0};break;
  case Kind::MonoBass: b.name="Mono sub";b.values={90,100,0,0,0,0};c.name="Centred low end";c.values={220,85,0,0,0,0};break;
  case Kind::Haas: b.name="Small side offset";b.values={7,100,0,0,0,0};c.name="Opposite-side offset";c.values={18,-100,0,0,0,0};break;
  case Kind::Gain: b.name="Minus 6 dB";b.values={-6,0,0,0,0,0};c.name="Plus 6 dB";c.values={6,0,0,0,0,0};break;
  case Kind::Polarity: b.name="Invert both";b.values={1,1,0,0,0,0};c.name="Normal both";c.values={0,0,0,0,0,0};break;
  case Kind::DCBlock: b.name="DC only";b.values[0]=2;c.name="Subsonic cleanup";c.values[0]=25;break;
  default: b.name="Factory";c.name="Factory";break;
  }
  // Never let a musical starting point exceed the processor's actual range.
  for(auto* preset:{&a,&b,&c}) for(int i=0;i<p.controlCount;++i)
    preset->values[i]=juce::jlimit(p.controls[i].min,p.controls[i].max,preset->values[i]);
  return {a,b,c};
}
} // namespace hungryghost
