// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#ifndef _VPLANET_NOISE_H_
#define _VPLANET_NOISE_H_

#include <memory>

#include "Curve.h"

class NoiseFunction {
public:
    NoiseFunction() {};
    virtual ~NoiseFunction() {};

    virtual double operator()(double x, double y) const = 0;
    virtual double operator()(double x, double y, double z) const = 0;
};

struct AngleCT {
    double cos, sin;
};

class Perlin : public NoiseFunction {
public:
    Perlin();
    virtual ~Perlin();

    // virtual double operator()(double x) const;
    virtual double operator()(double x, double y) const;
    virtual double operator()(double x, double y, double z) const;

private:
    static constexpr int NUM_PERMUTATIONS = 256;
    static_assert(NUM_PERMUTATIONS > 0 && NUM_PERMUTATIONS <= 256, "NUM_PERMUTATIONS must be between 1 and 256, inclusive");
    std::array<uint8_t, NUM_PERMUTATIONS> m_perms_x, m_perms_y, m_perms_z;
    std::array<AngleCT, NUM_PERMUTATIONS> m_thetas, m_phis;

    int hash2d(int i, int j) const;
    int hash3d(int i, int j, int k) const;

    static double fade(double t);
    static double lerp(double t, double a, double b);
};

class Turbulence : public NoiseFunction {
public:
    Turbulence(std::shared_ptr<NoiseFunction> base, int depth);
    virtual ~Turbulence();

    // virtual double operator()(double x) const;
    virtual double operator()(double x, double y) const;
    virtual double operator()(double x, double y, double z) const;

private:
    std::shared_ptr<NoiseFunction> m_base;
    int m_depth;
    // double m_persistence;
};

class Curve : public NoiseFunction {
public:
    Curve(std::shared_ptr<NoiseFunction> base, std::shared_ptr<CubicSpline> curve);
    virtual ~Curve();

    // virtual double operator()(double x) const;
    virtual double operator()(double x, double y) const;
    virtual double operator()(double x, double y, double z) const;

private:
    std::shared_ptr<NoiseFunction> m_base;
    std::shared_ptr<CubicSpline> m_curve;
};

#endif
