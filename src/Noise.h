// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#ifndef _VPLANET_NOISE_H_
#define _VPLANET_NOISE_H_

#include "Curve.h"

class PermutationTable {
public:
    PermutationTable();
    ~PermutationTable();

    unsigned char table[512];
};

class NoiseFunction {
public:
    NoiseFunction();
    virtual ~NoiseFunction();

    // double operator()(double x) const;
    virtual double operator()(double x, double y) const = 0;
    virtual double operator()(double x, double y, double z) const = 0;
};

class Perlin : public NoiseFunction {
public:
    Perlin();
    Perlin(double x_scale, double y_scale, double z_scale);
    virtual ~Perlin() noexcept;

    void setScales(double x, double y);
    void setScales(double x, double y, double z);

    // double operator()(double x) const;
    virtual double operator()(double x, double y) const;
    virtual double operator()(double x, double y, double z) const;

private:
    static double fade(double t);
    static double lerp(double t, double a, double b);

    static double grad(int hash, double x, double y);
    static double grad(int hash, double x, double y, double z);

    PermutationTable m_permutation;
    double m_x_scale, m_y_scale, m_z_scale;
};

class Octave : public NoiseFunction {
public:
    Octave(const NoiseFunction &base, int octaves, double persistence);
    virtual ~Octave();

    virtual double operator()(double x, double y) const;
    virtual double operator()(double x, double y, double z) const;

private:
    const NoiseFunction &m_noise;
    int m_octaves;
    double m_persistence;
};

class Curve : public NoiseFunction {
public:
    Curve(const NoiseFunction &base, const CubicSpline &curve);
    virtual ~Curve();

    virtual double operator()(double x, double y) const;
    virtual double operator()(double x, double y, double z) const;

private:
    const NoiseFunction &m_noise;
    const CubicSpline &m_curve;
};

#endif
