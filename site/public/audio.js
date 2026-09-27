const grid = document.querySelector('#demo-grid');
let context, current, intent;
const clock = seconds => `${Math.floor(seconds / 60)}:${String(Math.floor(seconds % 60)).padStart(2, '0')}`;

class Preview {
  constructor(demo) {
    this.demo = demo;
    this.offset = 0;
    this.processed = true;
    this.card = document.createElement('article');
    this.card.className = 'demo-card';
    // All text comes from the release's own demo manifest.
    const title = document.createElement('h3');
    title.textContent = demo.title;
    const description = document.createElement('p');
    description.textContent = demo.description;
    const source = document.createElement('span');
    source.className = 'demo-source';
    source.textContent = `${demo.source === 'drums' ? 'DRUM LOOP' : demo.source === 'plucks' ? 'SYNTH PLUCKS' : 'SYNTH CHORDS'} / 14 SEC`;
    this.card.append(source, title, description);
    this.card.insertAdjacentHTML('beforeend', `<div class="demo-controls"><button class="demo-play" type="button">Play</button><div class="demo-modes" role="group"><button type="button" aria-pressed="false">Dry</button><button type="button" aria-pressed="true">Processed</button></div></div><div class="demo-progress"><input type="range" min="0" max="${demo.duration}" value="0" step=".05"><span>0:00 / 0:14</span></div><p class="demo-status" role="status"></p>`);
    this.play = this.card.querySelector('.demo-play');
    this.play.setAttribute('aria-label', `Play ${demo.title} preview`);
    this.modes = [...this.card.querySelectorAll('.demo-modes button')];
    this.card.querySelector('.demo-modes').setAttribute('aria-label', `${demo.title} comparison`);
    this.slider = this.card.querySelector('input');
    this.slider.setAttribute('aria-label', `${demo.title} preview position`);
    this.time = this.card.querySelector('.demo-progress span');
    this.status = this.card.querySelector('.demo-status');
    this.play.addEventListener('click', () => this.playing ? this.pause() : this.start());
    this.modes.forEach((button, index) => button.addEventListener('click', () => {
      this.processed = index === 1;
      this.modes.forEach((mode, i) => mode.setAttribute('aria-pressed', String(i === index)));
      this.applyGains();
    }));
    this.slider.addEventListener('input', () => {
      const playing = this.playing;
      const wanted = Number(this.slider.value);
      this.pause();
      this.offset = wanted;
      this.update();
      if (playing && this.offset < this.demo.duration) this.start();
    });
    grid.append(this.card);
  }

  async start() {
    intent = this;
    if (current && current !== this) current.pause();
    this.play.disabled = true;
    this.status.textContent = 'Loading preview…';
    try {
      if (!context) {
        const Audio = window.AudioContext || window.webkitAudioContext;
        if (!Audio) throw Error('Audio previews are unavailable in this browser.');
        context = new Audio();
      }
      const resumed = context.resume();
      if (!this.loading) this.loading = Promise.all(['dry', 'wet'].map(async version => {
        const response = await fetch(`/audio/${this.demo.id}-${version}.mp3`);
        if (!response.ok) throw Error('The preview could not be loaded.');
        return context.decodeAudioData(await response.arrayBuffer());
      })).catch(error => { this.loading = null; throw error; });
      [this.buffers] = await Promise.all([this.loading, resumed]);
      if (intent !== this) return;
      if (this.offset >= this.demo.duration - .02) this.offset = 0;
      this.sources = this.buffers.map(buffer => {
        const source = context.createBufferSource();
        source.buffer = buffer;
        const gain = context.createGain();
        gain.gain.value = 0;
        source.connect(gain).connect(context.destination);
        source.onended = () => { source.disconnect(); gain.disconnect(); };
        return {source, gain};
      });
      this.started = context.currentTime;
      this.playing = true;
      current = this;
      this.applyGains();
      this.sources.forEach(({source}) => source.start(this.started, this.offset));
      this.play.textContent = 'Pause';
      this.play.setAttribute('aria-label', `Pause ${this.demo.title} preview`);
      this.status.textContent = '';
      this.tick();
    } catch (error) {
      this.status.textContent = error.message || 'Audio playback is unavailable in this browser.';
    } finally {
      this.play.disabled = false;
      if (intent !== this) this.status.textContent = '';
    }
  }

  applyGains() {
    if (!this.playing) return;
    this.sources.forEach(({gain}, i) => {
      const value = (i === 1) === this.processed ? 1 : 0;
      gain.gain.cancelScheduledValues(context.currentTime);
      gain.gain.setValueAtTime(gain.gain.value, context.currentTime);
      gain.gain.linearRampToValueAtTime(value, context.currentTime + .025);
    });
  }

  position() { return this.playing ? Math.min(this.demo.duration, this.offset + context.currentTime - this.started) : this.offset; }
  pause() {
    if (!this.playing) return;
    this.offset = this.position();
    this.playing = false;
    this.sources.forEach(({source, gain}) => {
      gain.gain.cancelScheduledValues(context.currentTime);
      gain.gain.setValueAtTime(gain.gain.value, context.currentTime);
      gain.gain.linearRampToValueAtTime(0, context.currentTime + .025);
      source.stop(context.currentTime + .03);
    });
    cancelAnimationFrame(this.frame);
    if (current === this) current = null;
    this.play.textContent = 'Play';
    this.play.setAttribute('aria-label', `Play ${this.demo.title} preview`);
    this.update();
  }
  update() {
    const position = this.position();
    this.slider.value = position;
    this.slider.setAttribute('aria-valuetext', `${clock(position)} of ${clock(this.demo.duration)}`);
    this.time.textContent = `${clock(position)} / ${clock(this.demo.duration)}`;
  }
  tick() {
    this.update();
    if (this.position() >= this.demo.duration) { this.pause(); return; }
    this.frame = requestAnimationFrame(() => this.tick());
  }
}

try {
  const response = await fetch('/audio/manifest.json');
  if (!response.ok) throw Error('The listening previews could not be loaded.');
  const manifest = await response.json();
  manifest.demos.forEach(demo => new Preview(demo));
} catch (error) {
  grid.textContent = error.message;
}
document.addEventListener('visibilitychange', () => { if (document.hidden) { intent = null; if (current) current.pause(); } });
