"""One reviewed catalogue drives the native products and storefront."""
from pathlib import Path
import json

ROOT = Path(__file__).resolve().parents[1]
def p(name, lo, hi, default, unit='', skew=1):
    return dict(name=name, minimum=lo, maximum=hi, default=default, unit=unit, skew=skew)

threshold = p('Threshold', -60, 0, -18, 'dB')
ratio = p('Ratio', 1, 20, 4, ':1')
attack = p('Attack', .1, 100, 10, 'ms', .4)
release = p('Release', 10, 1000, 120, 'ms', .4)
knee = p('Knee', 0, 24, 6, 'dB')
makeup = p('Makeup', -12, 24, 0, 'dB')
freq = lambda default: p('Frequency', 20, 20000, default, 'Hz', .23)
gain = lambda name='Gain', default=0: p(name, -18, 18, default, 'dB')
q = p('Resonance', .1, 12, .707, 'Q', .4)
drive = p('Drive', 0, 36, 9, 'dB')
tone = p('Tone', 200, 20000, 12000, 'Hz', .3)
unused = p('Character', 0, 100, 50, '%')
rows = []
def add(name, family, kind, description, controls, price=25):
    assert len(controls) <= 6
    rows.append(dict(id=name.lower(), name=name, family=family, engine=kind,
      description=description, price=price, controls=controls,
      status='development', version='0.1.0'))

add('REVERB','Space','OriginalReverb','Sixteen delay lines. One enormous sense of space.', [],25)
add('FERAL','Dynamics','OriginalFeral','Eight bands of dynamic EQ with a stereo bus compressor.', [],99)
rows[0].update(version='0.2.0',status='validated',image='/assets/reverb.png')
rows[1].update(version='0.1.1',status='validated',image='/assets/feral.png')
for name, kind, text, defaults in [
 ('RIFT','Compressor','A clean feed-forward compressor for precise level control.',[threshold,ratio,attack,release,knee,makeup]),
 ('CLAW','FastCompressor','Fast peak compression for drums and sharp attacks.',[p('Threshold',-60,0,-12,'dB'),ratio,p('Attack',.1,20,.5,'ms',.4),release,knee,makeup]),
 ('VELVET','RmsCompressor','Slower RMS detection for smooth vocal and instrument levelling.',[threshold,ratio,attack,release,knee,makeup]),
 ('BOND','BusCompressor','A stereo-linked compressor with a filtered detector.',[threshold,p('Ratio',1,10,2,':1'),attack,release,p('Key high-pass',20,500,100,'Hz',.4),makeup]),
 ('CRUSH','ParallelCompressor','Blend intense compression with the original signal.',[p('Threshold',-60,0,-30,'dB'),p('Ratio',1,20,12,':1'),attack,release,knee,makeup]),
 ('HUSH','DeEsser','Band-sensitive gain reduction for sibilance.',[threshold,freq(6500),p('Bandwidth',.3,5,1.5,'Q'),release,p('Range',0,24,6,'dB'),attack]),
 ('GATE','Gate','A downward gate with hysteresis and a controllable floor.',[p('Threshold',-80,0,-40,'dB'),p('Floor',-90,0,-70,'dB'),attack,release,p('Hysteresis',0,12,3,'dB'),p('Hold',0,500,40,'ms')]),
 ('BLOOM','Expander','Restore separation with gentle downward expansion.',[p('Threshold',-80,0,-35,'dB'),p('Ratio',1,8,2,':1'),attack,release,p('Range',0,60,30,'dB'),makeup]),
 ('PUNCH','Transient','Shape the attack and body of drums without a threshold.',[p('Attack',-100,100,30,'%'),p('Sustain',-100,100,-10,'%'),p('Speed',1,30,8,'ms'),p('Body',30,500,150,'ms'),p('Sensitivity',.2,5,1),makeup]),
 ('TAIL','Sustain','Extend or tighten note sustain with a slow envelope.',[p('Sustain',-100,100,30,'%'),p('Attack protect',0,100,70,'%'),p('Speed',1,30,8,'ms'),p('Body',30,1000,300,'ms'),p('Sensitivity',.2,5,1),makeup]),
 ('GUARD','Limiter','A linked sample-peak limiter with a hard safety ceiling.',[p('Ceiling',-12,0,-1,'dB'),p('Input',0,24,3,'dB'),release]),
 ('EDGE','Clipper','Set a peak ceiling with adjustable soft clipping.',[p('Ceiling',-18,0,-1,'dB'),p('Input',0,24,3,'dB'),p('Softness',0,100,25,'%')]),
 ('RIDE','Leveller','A slow automatic level rider with bounded gain.',[p('Target',-36,-6,-18,'dB'),p('Range',0,24,6,'dB'),p('Attack',50,2000,300,'ms'),p('Release',100,5000,1000,'ms'),p('Noise floor',-80,-20,-50,'dB')]),
 ('DUCK','Ducker','Use an external key to make room for another track.',[threshold,ratio,attack,release,p('Range',0,36,12,'dB')])]:
    add(name,'Dynamics',kind,text,defaults,39 if name in ('RIFT','VELVET','BOND','GUARD') else 25)

add('SHADE','Tone','ParametricEQ','Three fixed-width bell bands for broad tonal shaping.',[freq(120),gain('Low gain'),freq(1200),gain('Mid gain'),freq(8000),gain('High gain')],39)
add('TILT','Tone','TiltEQ','Move warmth and brightness around a single pivot.',[freq(1000),gain('Tilt')],15)
add('WEIGHT','Tone','LowShelf','A low shelf with an adjustable corner and resonance.',[freq(100),gain('Weight'),q],15)
add('AIR','Tone','HighShelf','A high shelf for presence, sparkle and darker tops.',[freq(8000),gain('Air'),q],15)
add('VOID','Tone','Notch','A focused rejection filter for problem frequencies.',[freq(1000),q],15)
add('CROSS','Tone','BandPass','Band-pass filtering with controllable width.',[freq(1000),q],15)
add('CONTOUR','Tone','Cuts','High-pass and low-pass filtering for focused bandwidth.',[p('Low cut',20,2000,80,'Hz',.3),p('High cut',500,20000,16000,'Hz',.4),q],15)
add('FOCUS','Tone','MidSideEQ','A bell EQ that independently shapes mid and side channels.',[freq(1000),gain('Mid gain'),gain('Side gain'),q],25)
for name,kind,text,params in [
 ('EMBER','SoftSaturation','Smooth symmetric saturation with tone filtering.',[drive,tone]),
 ('IRON','AsymmetricSaturation','Biased saturation with controllable even harmonics.',[drive,p('Bias',-.5,.5,.12),tone]),
 ('FURNACE','TubeSaturation','Two soft nonlinear stages for dense harmonic colour.',[drive,p('Character',0,100,35,'%'),tone]),
 ('FOLD','Wavefolder','Fold peaks back into the waveform for complex harmonics.',[drive,p('Symmetry',-.5,.5,0),tone]),
 ('RECT','Rectifier','Blend full-wave rectification into the original waveform.',[p('Amount',0,100,50,'%'),p('Bias',0,.5,.05),tone]),
 ('SHARDS','BitCrusher','Reduce amplitude resolution with optional dither.',[p('Bits',2,24,8,'bit'),p('Dither',0,100,0,'%'),tone]),
 ('STATIC','RateReducer','Sample-and-hold reduction with a smoothing filter.',[p('Rate',500,48000,8000,'Hz',.4),p('Smooth',200,20000,12000,'Hz',.3)])]:
 add(name,'Colour',kind,text,params,25)
for name,kind,text,ms,fb in [
 ('ECHO','Delay','A clean stereo delay with filtered feedback.',375,30),
 ('REEL','TapeDelay','A darkened delay with drifting tape-style repeats.',400,40),
 ('PING','PingPong','Repeats that cross from one channel to the other.',300,40),
 ('SLAP','SlapDelay','Short doubled reflections for vocals and instruments.',100,10),
 ('DUB','DubDelay','Saturated feedback with a darkened low-pass tone.',500,55),
 ('TEETH','Comb','A short feedback delay for pitched comb textures.',8,60)]:
 add(name,'Space',kind,text,[p('Time',.2 if name=='TEETH' else 1,50 if name=='TEETH' else 2000,ms,'ms',.35),p('Feedback',-95,95,fb,'%'),p('Tone',200,20000,6000,'Hz',.3),p('Motion',0,100,20 if name=='REEL' else 0,'%')],35)
for name,kind,text,params in [
 ('CHOIR','Chorus','Two modulated delay voices with stereo phase offset.',[p('Rate',.05,10,.7,'Hz',.4),p('Depth',0,100,40,'%'),p('Delay',5,40,18,'ms'),p('Feedback',-80,80,10,'%')]),
 ('FLUX','Flanger','A short modulated feedback delay with moving notches.',[p('Rate',.02,10,.2,'Hz',.4),p('Depth',0,100,60,'%'),p('Delay',.2,10,2,'ms'),p('Feedback',-90,90,40,'%')]),
 ('PHASE','Phaser','Six moving all-pass stages with feedback.',[p('Rate',.02,10,.3,'Hz',.4),p('Depth',0,100,60,'%'),freq(800),p('Feedback',-80,80,20,'%')]),
 ('PULSE','Tremolo','Amplitude modulation from gentle motion to hard pulses.',[p('Rate',.05,30,4,'Hz',.4),p('Depth',0,100,60,'%'),p('Shape',0,100,0,'%')]),
 ('ORBIT','AutoPan','A moving stereo balance with adjustable motion shape.',[p('Rate',.05,20,.5,'Hz',.4),p('Depth',0,100,80,'%'),p('Shape',0,100,0,'%')]),
 ('RING','RingMod','Multiply the signal by an oscillator for metallic sidebands.',[p('Frequency',.1,4000,100,'Hz',.3),p('Depth',0,100,100,'%')]),
 ('DRIFT','Vibrato','Modulated fractional delay for pitch movement.',[p('Rate',.05,20,5,'Hz',.4),p('Depth',0,100,30,'%'),p('Delay',2,30,10,'ms')])]:
 add(name,'Motion',kind,text,params,25)
add('WIDE','Stereo','Width','Control side energy while keeping the mid signal intact.',[p('Width',0,200,125,'%'),p('Bass mono',20,1000,120,'Hz',.4)],15)
add('CENTER','Stereo','MonoBass','Collapse only the low-frequency side information.',[p('Crossover',20,1000,120,'Hz',.4),p('Amount',0,100,100,'%')],15)
add('SHADOW','Stereo','Haas','A small channel delay for the Haas widening effect.',[p('Delay',0,40,12,'ms'),p('Side',-100,100,100,'%')],15)
add('TRIM','Utility','Gain','Precise gain, stereo balance and channel metering.',[p('Gain',-60,24,0,'dB'),p('Balance',-100,100,0,'%')],15)
add('REVERSE','Utility','Polarity','Independent left and right polarity inversion.',[p('Left invert',0,1,1),p('Right invert',0,1,0)],15)
add('CLEAN','Utility','DCBlock','Remove DC offset and subsonic energy.',[p('Cutoff',1,40,10,'Hz',.5)],15)

assert len(rows)==50, len(rows)
assert len({r['id'] for r in rows})==50
(ROOT/'site/public/catalogue.json').write_text(json.dumps(rows,indent=2)+'\n')
(ROOT/'catalogue.json').write_text(json.dumps(rows,indent=2)+'\n')
lines=['#pragma once','#include <array>','#include <string_view>','namespace hungryghost {',
 'enum class Kind { '+', '.join(r['engine'] for r in rows)+' };',
 'struct Control { const char* name; float min, max, initial; const char* unit; float skew; };',
 'struct Product { const char* id; const char* name; const char* family; const char* description; Kind kind; int controlCount; std::array<Control,6> controls; };',
 'inline constexpr std::array<Product,50> products {{']
for r in rows:
 cs=[f'{{"{c["name"]}",{c["minimum"]}f,{c["maximum"]}f,{c["default"]}f,"{c["unit"]}",{c["skew"]}f}}' for c in r['controls']]
 # C++ float literals need a decimal point.
 import re
 cs=[re.sub(r'(?<![\w.])(-?\d+)f',r'\1.0f',s) for s in cs]
 cs+=['{"",0.f,1.f,0.f,"",1.f}']*(6-len(cs))
 lines.append('{"'+r['id']+'","'+r['name']+'","'+r['family']+'","'+r['description']+'",Kind::'+r['engine']+','+str(len(r['controls']))+',{{'+','.join(cs)+'}}},')
lines+=['}};','}']
(ROOT/'Source/Catalogue.h').write_text('\n'.join(lines)+'\n')
cm=['set(HG_PRODUCT_TARGETS)']
for i,r in enumerate(rows[2:],2):
 cm.append(f'hg_add_product({r["name"]} {i} Hg{i:02d} "{r["family"]}")')
(ROOT/'Products.cmake').write_text('\n'.join(cm)+'\n')
print('Catalogue: 50 products, 48 additional distinct engines.')
