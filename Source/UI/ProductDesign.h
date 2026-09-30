#pragma once
#include "../Catalogue.h"
#include <juce_graphics/juce_graphics.h>

namespace hungryghost {
enum class EditorLayout {
  Precision, Rack, Optical, Bus, Parallel, Gate, Transient, Peak, Rider,
  Console, Filter, Drive, Digital, Echo, Tape, PingPong, Dub, Comb,
  Modulation, Phase, Pan, Ring, Stereo, Trim, Polarity, Clean
};
struct ProductDesign {
  EditorLayout layout;
  int width, height;
  juce::Colour accent, metal;
  const char* instrument;
};
inline ProductDesign designFor(Kind kind) {
  using L = EditorLayout;
  switch (kind) {
  case Kind::Compressor: return {L::Precision,1000,640,juce::Colour(0xffe6b879),juce::Colour(0xff202822),"PRECISION DYNAMICS"};
  case Kind::FastCompressor: return {L::Rack,1060,510,juce::Colour(0xffd69470),juce::Colour(0xff252525),"PEAK COMPRESSION"};
  case Kind::RmsCompressor: return {L::Optical,900,600,juce::Colour(0xffd4bf89),juce::Colour(0xff353831),"RMS LEVELLING"};
  case Kind::BusCompressor: return {L::Bus,1160,720,juce::Colour(0xffe4b976),juce::Colour(0xff3c493a),"STEREO BUS COMPRESSION"};
  case Kind::ParallelCompressor: return {L::Parallel,1000,640,juce::Colour(0xffe2a57c),juce::Colour(0xff302821),"PARALLEL COMPRESSION"};
  case Kind::DeEsser: return {L::Gate,960,610,juce::Colour(0xffd5bb7d),juce::Colour(0xff303025),"SIBILANCE CONTROL"};
  case Kind::Gate: return {L::Gate,960,610,juce::Colour(0xffa6c98d),juce::Colour(0xff222f27),"HYSTERESIS GATE"};
  case Kind::Expander: return {L::Precision,1000,640,juce::Colour(0xffa8cfa7),juce::Colour(0xff25312b),"DOWNWARD EXPANSION"};
  case Kind::Transient: return {L::Transient,920,640,juce::Colour(0xffefab72),juce::Colour(0xff32271e),"TRANSIENT SHAPING"};
  case Kind::Sustain: return {L::Transient,920,640,juce::Colour(0xffc5be96),juce::Colour(0xff2d2e26),"SUSTAIN SHAPING"};
  case Kind::Limiter: return {L::Peak,800,640,juce::Colour(0xffe8ce8c),juce::Colour(0xff272b25),"SAMPLE-PEAK LIMITER"};
  case Kind::Clipper: return {L::Peak,800,640,juce::Colour(0xffe58e80),juce::Colour(0xff302422),"SOFT / HARD CLIPPER"};
  case Kind::Leveller: return {L::Rider,860,640,juce::Colour(0xffaebda7),juce::Colour(0xff2b3029),"AUTOMATIC LEVEL RIDER"};
  case Kind::Ducker: return {L::Parallel,1000,640,juce::Colour(0xffc4a67c),juce::Colour(0xff2c2921),"EXTERNAL-KEY DUCKING"};
  case Kind::ParametricEQ: return {L::Console,1040,670,juce::Colour(0xff94c6de),juce::Colour(0xff263039),"THREE-BAND TONE CONSOLE"};
  case Kind::TiltEQ: return {L::Filter,900,590,juce::Colour(0xffa5cbbd),juce::Colour(0xff293630),"WARMTH / BRIGHTNESS"};
  case Kind::LowShelf: return {L::Filter,940,610,juce::Colour(0xff90b3d0),juce::Colour(0xff222e3a),"LOW-SHELF EQUALIZER"};
  case Kind::HighShelf: return {L::Filter,940,610,juce::Colour(0xffc6dbe2),juce::Colour(0xff303b41),"HIGH-SHELF EQUALIZER"};
  case Kind::Notch: return {L::Filter,900,590,juce::Colour(0xffb0a4d3),juce::Colour(0xff292432),"REJECTION FILTER"};
  case Kind::BandPass: return {L::Filter,900,590,juce::Colour(0xff8ebdc9),juce::Colour(0xff233236),"BAND-PASS FILTER"};
  case Kind::Cuts: return {L::Console,1040,670,juce::Colour(0xff99c3d4),juce::Colour(0xff29333a),"BANDWIDTH CONSOLE"};
  case Kind::MidSideEQ: return {L::Console,1040,670,juce::Colour(0xff9fb2e0),juce::Colour(0xff262c3e),"MID / SIDE EQUALIZER"};
  case Kind::SoftSaturation: return {L::Drive,800,610,juce::Colour(0xffeea374),juce::Colour(0xff392a20),"SYMMETRIC SATURATION"};
  case Kind::AsymmetricSaturation: return {L::Drive,840,650,juce::Colour(0xffbc9c88),juce::Colour(0xff35302b),"BIASED SATURATION"};
  case Kind::TubeSaturation: return {L::Drive,840,650,juce::Colour(0xffedbd78),juce::Colour(0xff383027),"TWO-STAGE SATURATION"};
  case Kind::Wavefolder: return {L::Digital,920,620,juce::Colour(0xffce9fe3),juce::Colour(0xff30283a),"WAVE FOLDING"};
  case Kind::Rectifier: return {L::Digital,920,620,juce::Colour(0xffe8aa9c),juce::Colour(0xff342827),"FULL-WAVE RECTIFICATION"};
  case Kind::BitCrusher: return {L::Digital,860,610,juce::Colour(0xffb4cd83),juce::Colour(0xff293021),"AMPLITUDE QUANTIZATION"};
  case Kind::RateReducer: return {L::Digital,860,610,juce::Colour(0xff9cc3b1),juce::Colour(0xff24322b),"SAMPLE-HOLD REDUCTION"};
  case Kind::Delay: return {L::Echo,1000,610,juce::Colour(0xff87cfc2),juce::Colour(0xff21312f),"FEEDBACK DELAY"};
  case Kind::TapeDelay: return {L::Tape,1000,650,juce::Colour(0xffd1bc90),juce::Colour(0xff33302a),"SATURATING TAPE DELAY"};
  case Kind::PingPong: return {L::PingPong,1000,610,juce::Colour(0xff9ebcdb),juce::Colour(0xff25303a),"ALTERNATING STEREO DELAY"};
  case Kind::SlapDelay: return {L::Echo,900,570,juce::Colour(0xffc8bea0),juce::Colour(0xff343329),"SHORT STEREO SLAP"};
  case Kind::DubDelay: return {L::Dub,960,670,juce::Colour(0xffc6cf89),juce::Colour(0xff2d3325),"FILTERED FEEDBACK DELAY"};
  case Kind::Comb: return {L::Comb,880,600,juce::Colour(0xffaca0d1),juce::Colour(0xff2a2736),"COMB RESONATOR"};
  case Kind::Chorus: return {L::Modulation,920,640,juce::Colour(0xffb8a7e1),juce::Colour(0xff2b2639),"MODULATED ENSEMBLE"};
  case Kind::Flanger: return {L::Modulation,920,640,juce::Colour(0xff91c5c9),juce::Colour(0xff263638),"SHORT-DELAY MODULATION"};
  case Kind::Phaser: return {L::Phase,920,640,juce::Colour(0xffd2a9d1),juce::Colour(0xff352937),"ALL-PASS PHASE SHIFT"};
  case Kind::Tremolo: return {L::Pan,860,610,juce::Colour(0xffb9ce92),juce::Colour(0xff2d3527),"AMPLITUDE MODULATION"};
  case Kind::AutoPan: return {L::Pan,860,610,juce::Colour(0xffc0afe3),juce::Colour(0xff302b3c),"STEREO PAN MODULATION"};
  case Kind::RingMod: return {L::Ring,840,610,juce::Colour(0xffd0b88d),juce::Colour(0xff342f27),"OSCILLATOR MULTIPLICATION"};
  case Kind::Vibrato: return {L::Modulation,920,640,juce::Colour(0xff9dbcdc),juce::Colour(0xff253142),"FRACTIONAL-DELAY VIBRATO"};
  case Kind::Width: return {L::Stereo,860,600,juce::Colour(0xffb5cd82),juce::Colour(0xff2f3527),"MID / SIDE WIDTH"};
  case Kind::MonoBass: return {L::Stereo,860,600,juce::Colour(0xffa9c4b0),juce::Colour(0xff29392f),"LOW-FREQUENCY MONO"};
  case Kind::Haas: return {L::Stereo,860,600,juce::Colour(0xffb5afce),juce::Colour(0xff302d3b),"CHANNEL-DELAY WIDTH"};
  case Kind::Gain: return {L::Trim,660,550,juce::Colour(0xffb8c5cc),juce::Colour(0xff2b3338),"GAIN / BALANCE"};
  case Kind::Polarity: return {L::Polarity,680,470,juce::Colour(0xffd0b8a0),juce::Colour(0xff34312d),"CHANNEL POLARITY"};
  case Kind::DCBlock: return {L::Clean,660,500,juce::Colour(0xffa4c8c5),juce::Colour(0xff293533),"DC / SUBSONIC FILTER"};
  default: return {L::Precision,1000,640,juce::Colour(0xffa2e4cf),juce::Colour(0xff202822),"HUNGRY GHOST AUDIO"};
  }
}
} // namespace hungryghost
