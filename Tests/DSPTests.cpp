#include "DSP/SuiteEngine.h"
#include <cmath>
#include <iostream>
#include <numeric>
#include <stdexcept>
using namespace hungryghost;
void require(bool x, const char *reason) {
  if (!x)
    throw std::runtime_error(reason);
}
std::array<float, 6> settings(const Product &p) {
  std::array<float, 6> v{};
  for (int j = 0; j < p.controlCount; ++j)
    v[j] = p.controls[j].initial;
  return v;
}
double rms(const std::vector<float> &v) {
  double a = 0;
  for (float x : v)
    a += x * x;
  return std::sqrt(a / v.size());
}
void tone(std::vector<float> &l, std::vector<float> &r, double sr, double hz,
          float amplitude) {
  for (std::size_t i = 0; i < l.size(); ++i) {
    l[i] = amplitude *
           static_cast<float>(std::sin(6.283185307179586 * hz * i / sr));
    r[i] = l[i] * .7f;
  }
}
int main() {
  try {
    for (std::size_t index = 2; index < products.size(); ++index) {
      const auto &p = products[index];
      for (double sr : {22050., 44100., 48000., 96000., 192000.}) {
        SuiteEngine e(p.kind);
        e.prepare(sr);
        auto v = settings(p);
        for (int pass = 0; pass < 3; ++pass) {
          if (pass > 0)
            for (int j = 0; j < p.controlCount; ++j)
              v[j] = pass == 1 ? p.controls[j].min : p.controls[j].max;
          e.setControls(v, 1, 0);
          std::vector<float> l(8193), r(8193), key(8193, .5f);
          tone(l, r, sr, 777, .2f);
          e.process(l.data(), r.data(), key.data(), key.data(),
                    static_cast<int>(l.size()));
          for (float x : l)
            require(std::isfinite(x) && std::abs(x) < 1000,
                    "Invalid or unbounded DSP output");
          for (float x : r)
            require(std::isfinite(x) && std::abs(x) < 1000,
                    "Invalid stereo DSP output");
          e.reset();
        }
        // Zero-sized and mono host blocks must remain valid.
        float sample = .1f;
        e.process(&sample, nullptr, nullptr, nullptr, 0);
        e.process(&sample, nullptr, nullptr, nullptr, 1);
        require(std::isfinite(sample), "Invalid mono result");
      }
      std::cout
          << p.name
          << " finite stereo/mono at five sample rates, full parameter range\n";
    }
    std::vector<float> l(48000), r(48000);
    tone(l, r, 48000, 1000, .8f);
    SuiteEngine gain(Kind::Gain);
    gain.prepare(48000);
    gain.setControls({-6, 0, 0, 0, 0, 0}, 1, 0);
    gain.reset();
    gain.process(l.data(), r.data(), nullptr, nullptr, 48000);
    require(std::abs(rms(l) - .8 / std::sqrt(2.) * std::pow(10., -.3)) < .0001,
            "TRIM gain calibration");
    SuiteEngine dc(Kind::DCBlock);
    dc.prepare(48000);
    std::fill(l.begin(), l.end(), .5f);
    std::fill(r.begin(), r.end(), -.5f);
    dc.process(l.data(), r.data(), nullptr, nullptr, 48000);
    require(std::abs(l.back()) < .00001, "DC removal");
    SuiteEngine notch(Kind::Notch);
    notch.prepare(48000);
    notch.setControls({1000, .707f, 0, 0, 0, 0}, 1, 0);
    notch.reset();
    tone(l, r, 48000, 1000, .5f);
    notch.process(l.data(), r.data(), nullptr, nullptr, 48000);
    double tail = 0;
    for (std::size_t i = 24000; i < l.size(); ++i)
      tail += l[i] * l[i];
    require(std::sqrt(tail / 24000) < .001, "Notch rejection");
    SuiteEngine compressor(Kind::Compressor);
    compressor.prepare(48000);
    compressor.setControls({-20, 4, 1, 100, 0, 0}, 1, 0);
    compressor.reset();
    tone(l, r, 48000, 1000, .5f);
    compressor.process(l.data(), r.data(), nullptr, nullptr, 48000);
    require(compressor.gainReduction() > 8 && compressor.gainReduction() < 12,
            "Compression ratio calibration");
    SuiteEngine limiter(Kind::Limiter);
    limiter.prepare(48000);
    limiter.setControls({-1, 12, 100, 0, 0, 0}, 1, 0);
    limiter.reset();
    tone(l, r, 48000, 1000, .8f);
    limiter.process(l.data(), r.data(), nullptr, nullptr, 48000);
    for (float x : l)
      require(std::abs(x) <= std::pow(10.f, -.05f) + .00001f,
              "Limiter peak ceiling");
    SuiteEngine echo(Kind::Delay);
    echo.prepare(48000);
    echo.setControls({100, 0, 12000, 0, 0, 0}, 1, 0);
    echo.reset();
    std::fill(l.begin(), l.end(), 0);
    std::fill(r.begin(), r.end(), 0);
    l[0] = 1;
    echo.process(l.data(), r.data(), nullptr, nullptr, 48000);
    require(std::abs(l[4800] - 1) < .0001 && std::abs(l[4799]) < .0001,
            "Delay time calibration");
    // Processing must be independent of host block boundaries.
    for (std::size_t index = 2; index < products.size(); ++index) {
      SuiteEngine a(products[index].kind), b(products[index].kind);
      a.prepare(48000);
      b.prepare(48000);
      std::vector<float> al(14000), ar(14000), bl, br;
      tone(al, ar, 48000, 470, .1f);
      bl = al;
      br = ar;
      a.process(al.data(), ar.data(), nullptr, nullptr, 14000);
      for (int offset = 0; offset < 14000; offset += 137)
        b.process(bl.data() + offset, br.data() + offset, nullptr, nullptr,
                  std::min(137, 14000 - offset));
      for (int i = 0; i < 14000; ++i)
        require(std::abs(al[i] - bl[i]) < .000001f &&
                    std::abs(ar[i] - br[i]) < .000001f,
                "Host block size changes DSP");
    }
    std::cout << "PASS: 48 engines, DSP invariants and calibrated behaviours\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "FAIL: " << e.what() << '\n';
    return 1;
  }
}
