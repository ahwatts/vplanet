// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <cmath>
#include <iostream>
#include <random>

#include "Noise.h"

double reduceToRange(double x, double modulus);

PermutationTable::PermutationTable() {
    std::random_device seed;
    std::default_random_engine engine{seed()};

    for (int i = 0; i < 256; ++i) {
        table[i] = i;
    }

    std::uniform_int_distribution<int> random_int{0, std::numeric_limits<int>::max()};
    for (int i = 255; i > 0; --i) {
        int sucker = random_int(engine) % (i+1);
        std::swap(table[sucker], table[i]);
    }

    for (int i = 256; i < 512; ++i) {
        table[i] = table[i-256];
    }
}

PermutationTable::~PermutationTable() {}

NoiseFunction::NoiseFunction() {}

NoiseFunction::~NoiseFunction() {}

Perlin::Perlin()
    : m_permutation{},
      m_x_scale{1.0},
      m_y_scale{1.0},
      m_z_scale{1.0}
{}

Perlin::Perlin(double x_scale, double y_scale, double z_scale)
    : m_permutation{},
      m_x_scale{x_scale},
      m_y_scale{y_scale},
      m_z_scale{z_scale}
{}

Perlin::~Perlin() {}

void Perlin::setScales(double x, double y) {
    m_x_scale = x;
    m_y_scale = y;
}

void Perlin::setScales(double x, double y, double z) {
    m_x_scale = x;
    m_y_scale = y;
    m_z_scale = z;
}

double Perlin::operator()(double xx, double yy) const {
    const unsigned char *const p = m_permutation.table;

    double x = reduceToRange(xx * m_x_scale, 256.0);
    double y = reduceToRange(yy * m_y_scale, 256.0);

    int xa = static_cast<int>(std::floor(x));
    int xb = (xa + 1) % 256;
    int ya = static_cast<int>(std::floor(y));
    int yb = (ya + 1) % 256;
    double xf = x - xa;
    double yf = y - ya;

    double u = Perlin::fade(xf);
    double v = Perlin::fade(yf);

    int aa = p[p[xa] + ya];
    int ab = p[p[xa] + yb];
    int ba = p[p[xb] + ya];
    int bb = p[p[xb] + yb];

    double x1 = lerp(u, grad(aa, xf, yf),   grad(ba, xf-1, yf));
    double x2 = lerp(u, grad(ab, xf, yf-1), grad(bb, xf-1, yf-1));
    double rv = lerp(v, x1, x2);
    return rv;
}

double Perlin::operator()(double xx, double yy, double zz) const {
    const unsigned char *const p = m_permutation.table;

    double x = reduceToRange(xx * m_x_scale, 256.0);
    double y = reduceToRange(yy * m_y_scale, 256.0);
    double z = reduceToRange(zz * m_z_scale, 256.0);

    int xa = static_cast<int>(std::floor(x));
    int xb = (xa + 1) % 256;
    int ya = static_cast<int>(std::floor(y));
    int yb = (ya + 1) % 256;
    int za = static_cast<int>(std::floor(z));
    int zb = (za + 1) % 256;

    double xf = x - xa;
    double yf = y - ya;
    double zf = z - za;

    double u = fade(xf);
    double v = fade(yf);
    double w = fade(zf);

    int aaa = p[p[p[xa] + ya] + za];
    int aab = p[p[p[xa] + ya] + zb];
    int aba = p[p[p[xa] + yb] + za];
    int abb = p[p[p[xa] + yb] + zb];
    int baa = p[p[p[xb] + ya] + za];
    int bab = p[p[p[xb] + ya] + zb];
    int bba = p[p[p[xb] + yb] + za];
    int bbb = p[p[p[xb] + yb] + zb];

    double x1, y1, x2, y2;
    x1 = lerp(u, grad(aaa, xf, yf, zf),   grad(baa, xf-1, yf, zf));
    x2 = lerp(u, grad(aba, xf, yf-1, zf), grad(bba, xf-1, yf-1, zf));
    y1 = lerp(v, x1, x2);

    x1 = lerp(u, grad(aab, xf, yf, zf-1),   grad(bab, xf-1, yf, zf-1));
    x2 = lerp(u, grad(abb, xf, yf-1, zf-1), grad(bbb, xf-1, yf-1, zf-1));
    y2 = lerp(v, x1, x2);

    double rv = lerp(w, y1, y2);
    static double min_rv = rv, max_rv = rv;
    min_rv = std::min(rv, min_rv);
    max_rv = std::max(rv, max_rv);
    return rv;
}

double Perlin::fade(double t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double Perlin::lerp(double t, double a, double b) {
    return t*(b - a) + a;
}

double Perlin::grad(int hash, double x, double y) {
    switch (hash & 0x3) {
        case 0x0: return  x +  y;
        case 0x1: return -x +  y;
        case 0x2: return  x + -y;
        case 0x3: return -x + -y;
        default : return 0;
    }
}

double Perlin::grad(int hash, double x, double y, double z) {
    switch (hash & 0xF) {
        case 0x0: return  x +  y;
        case 0x1: return -x +  y;
        case 0x2: return  x + -y;
        case 0x3: return -x + -y;
        case 0x4: return  x +  z;
        case 0x5: return -x +  z;
        case 0x6: return  x + -z;
        case 0x7: return -x + -z;
        case 0x8: return  y +  z;
        case 0x9: return -y +  z;
        case 0xA: return  y + -z;
        case 0xB: return -y + -z;
        case 0xC: return  x +  y;
        case 0xD: return -x +  y;
        case 0xE: return -y +  z;
        case 0xF: return -y + -z;
        default: return 0;
    }
}

Octave::Octave(const NoiseFunction &base, int octaves, double persistence)
    : m_noise{base},
      m_octaves{octaves},
      m_persistence{persistence}
{}

Octave::~Octave() {}

double Octave::operator()(double x, double y) const {
    double total = 0;
    double frequency = 1;
    double amplitude = 1;
    double max_value = 0;

    for (int i = 0; i < m_octaves; ++i) {
        total += m_noise(x * frequency, y * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= m_persistence;
        frequency *= 2;
    }

    double rv = total / max_value;
    return rv;
}

double Octave::operator()(double x, double y, double z) const {
    double total = 0;
    double frequency = 1;
    double amplitude = 1;
    double max_value = 0;

    for (int i = 0; i < m_octaves; ++i) {
        total += m_noise(x * frequency, y * frequency, z * frequency) * amplitude;
        max_value += amplitude;
        amplitude *= m_persistence;
        frequency *= 2;
    }

    double rv = total / max_value;
    return rv;
}

Curve::Curve(const NoiseFunction &base, const CubicSpline &curve)
    : m_noise{base},
      m_curve{curve}
{}

Curve::~Curve() {}

double Curve::operator()(double x, double y) const {
    double rv = m_curve(m_noise(x, y));
    return rv;
}

double Curve::operator()(double x, double y, double z) const {
    double rv = m_curve(m_noise(x, y, z));
    return rv;
}

double reduceToRange(double x, double modulus) {
    while (x >= modulus) {
        x -= modulus;
    }

    while (x < 0) {
        x += modulus;
    }

    return x;
}
