// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <algorithm>
#include <cmath>
#include <memory>
#include <numbers>
#include <random>

#include "glm.h"

#include "Noise.h"

double rem_euclid(double x, double y) {
    double r = std::fmod(x, y);
    if (r < 0.0) {
        return r + std::abs(y);
    } else {
        return r;
    }
}

Perlin::Perlin()
: m_perms_x{},
  m_perms_y{},
  m_perms_z{},
  m_thetas{},
  m_phis{}
{
    std::random_device seed;
    std::default_random_engine engine{seed()};
    std::uniform_real_distribution<double> theta_gen{0.0, 2.0*std::numbers::pi};
    std::uniform_real_distribution<double> phi_gen{-1.0, 1.0};

    for (int i = 0; i < NUM_PERMUTATIONS; ++i) {
        m_perms_x[i] = i;
        m_perms_y[i] = i;
        m_perms_z[i] = i;

        double theta = theta_gen(engine);
        m_thetas[i].cos = std::cos(theta);
        m_thetas[i].sin = std::sin(theta);

        m_phis[i].cos = phi_gen(engine);
        m_phis[i].sin = std::sin(std::acos(m_phis[i].cos));
    }
    std::shuffle(m_perms_x.begin(), m_perms_x.end(), engine);
    std::shuffle(m_perms_y.begin(), m_perms_y.end(), engine);
    std::shuffle(m_perms_z.begin(), m_perms_z.end(), engine);
}

Perlin::~Perlin() {}

double Perlin::operator()(double xx, double yy) const {
    double xf = std::floor(xx), yf = std::floor(yy);
    double xt = xx - xf, yt = yy - yf;

    int x0 = static_cast<int>(std::floor(rem_euclid(xf, NUM_PERMUTATIONS - 1)));
    int y0 = static_cast<int>(std::floor(rem_euclid(yf, NUM_PERMUTATIONS - 1)));
    int x1 = (x0 + 1) % (NUM_PERMUTATIONS - 1);
    int y1 = (y0 + 1) % (NUM_PERMUTATIONS - 1);

    glm::dvec2 p00{xt, yt};
    glm::dvec2 p01{xt, yt - 1};
    glm::dvec2 p10{xt - 1, yt};
    glm::dvec2 p11{xt - 1, yt - 1};

    const AngleCT &theta00 = m_thetas[hash2d(x0, y0)];
    const AngleCT &theta01 = m_thetas[hash2d(x0, y1)];
    const AngleCT &theta10 = m_thetas[hash2d(x1, y0)];
    const AngleCT &theta11 = m_thetas[hash2d(x1, y1)];

    glm::dvec2 g00{theta00.cos, theta00.sin};
    glm::dvec2 g01{theta01.cos, theta01.sin};
    glm::dvec2 g10{theta10.cos, theta10.sin};
    glm::dvec2 g11{theta11.cos, theta11.sin};

    double v00 = glm::dot(p00, g00);
    double v01 = glm::dot(p01, g01);
    double v10 = glm::dot(p10, g10);
    double v11 = glm::dot(p11, g11);

    double u = fade(xt), v = fade(yt);

    double v0 = lerp(u, v00, v10);
    double v1 = lerp(u, v01, v11);
    return lerp(v, v0, v1);
}

double Perlin::operator()(double xx, double yy, double zz) const {
    double xf = std::floor(xx), yf = std::floor(yy), zf = std::floor(zz);
    double xt = xx - xf, yt = yy - yf, zt = zz - zf;

    int x0 = static_cast<int>(std::floor(rem_euclid(xf, NUM_PERMUTATIONS - 1)));
    int y0 = static_cast<int>(std::floor(rem_euclid(yf, NUM_PERMUTATIONS - 1)));
    int z0 = static_cast<int>(std::floor(rem_euclid(zf, NUM_PERMUTATIONS - 1)));
    int x1 = (x0 + 1) % (NUM_PERMUTATIONS - 1);
    int y1 = (y0 + 1) % (NUM_PERMUTATIONS - 1);
    int z1 = (z0 + 1) % (NUM_PERMUTATIONS - 1);

    double u = fade(xt), v = fade(yt), w = fade(zt);

    // Compute the values for the two z faces.
    double v0 = 0.0, v1 = 0.0;
    for (int k = 0; k < 2; ++k) {

        // Compute the values for the two y edges of this z face.
        double v0x = 0.0, v1x = 0.0;
        for (int j = 0; j < 2; ++j) {

            // Compute the values for the two x-corners of this yz edge.
            double v0xx = 0.0, v1xx = 0.0;
            for (int i = 0; i < 2; ++i) {
                int xc = (i == 0) ? x0 : x1;
                int yc = (j == 0) ? y0 : y1;
                int zc = (k == 0) ? z0 : z1;

                // p is the vector from this corner to the point.
                glm::dvec3 p{xt - i, yt - j, zt - k};

                // Get the random gradient direction for this corner.
                const AngleCT &theta = m_thetas[hash3d(xc, yc, zc)];
                const AngleCT &phi = m_phis[hash3d(xc, yc, zc)];
                glm::dvec3 g{phi.sin * theta.cos, phi.sin * theta.sin, phi.cos};

                // Compute the value for this point (vector-to-point dot unit-gradient)
                double val = glm::dot(p, g);

                if (i == 0) {
                    v0xx = val;
                } else {
                    v1xx = val;
                }
            }

            // Lerp the values for each corner based on the faded x value.
            if (j == 0) {
                v0x = lerp(u, v0xx, v1xx);
            } else {
                v1x = lerp(u, v0xx, v1xx);
            }
        }

        // Lerp the values for each edge based on the faded y value.
        if (k == 0) {
            v0 = lerp(v, v0x, v1x);
        } else {
            v1 = lerp(v, v0x, v1x);
        }
    }

    // Lerp the values for the two z faces based on the faded z value.
    return lerp(w, v0, v1);

    // glm::dvec3 p000{xt, yt, zt};
    // glm::dvec3 p001{xt, yt, zt - 1};
    // glm::dvec3 p010{xt, yt - 1, zt};
    // glm::dvec3 p011{xt, yt - 1, zt - 1};
    // glm::dvec3 p100{xt - 1, yt, zt};
    // glm::dvec3 p101{xt - 1, yt, zt - 1};
    // glm::dvec3 p110{xt - 1, yt - 1, zt};
    // glm::dvec3 p111{xt - 1, yt - 1, zt - 1};

    // const AngleCT &theta000 = m_thetas[hash3d(x0, y0, z0)];
    // const AngleCT &theta001 = m_thetas[hash3d(x0, y0, z1)];
    // const AngleCT &theta010 = m_thetas[hash3d(x0, y1, z0)];
    // const AngleCT &theta011 = m_thetas[hash3d(x0, y1, z1)];
    // const AngleCT &theta100 = m_thetas[hash3d(x1, y0, z0)];
    // const AngleCT &theta101 = m_thetas[hash3d(x1, y0, z1)];
    // const AngleCT &theta110 = m_thetas[hash3d(x1, y1, z0)];
    // const AngleCT &theta111 = m_thetas[hash3d(x1, y1, z1)];

    // const AngleCT &phi000 = m_phis[hash3d(x0, y0, z0)];
    // const AngleCT &phi001 = m_phis[hash3d(x0, y0, z1)];
    // const AngleCT &phi010 = m_phis[hash3d(x0, y1, z0)];
    // const AngleCT &phi011 = m_phis[hash3d(x0, y1, z1)];
    // const AngleCT &phi100 = m_phis[hash3d(x1, y0, z0)];
    // const AngleCT &phi101 = m_phis[hash3d(x1, y0, z1)];
    // const AngleCT &phi110 = m_phis[hash3d(x1, y1, z0)];
    // const AngleCT &phi111 = m_phis[hash3d(x1, y1, z1)];

    // glm::dvec3 g000{phi000.sin * theta000.cos, phi000.sin * theta000.sin, phi000.cos};
    // glm::dvec3 g001{phi001.sin * theta001.cos, phi001.sin * theta001.sin, phi001.cos};
    // glm::dvec3 g010{phi010.sin * theta010.cos, phi010.sin * theta010.sin, phi010.cos};
    // glm::dvec3 g011{phi011.sin * theta011.cos, phi011.sin * theta011.sin, phi011.cos};
    // glm::dvec3 g100{phi100.sin * theta100.cos, phi100.sin * theta100.sin, phi100.cos};
    // glm::dvec3 g101{phi101.sin * theta101.cos, phi101.sin * theta101.sin, phi101.cos};
    // glm::dvec3 g110{phi110.sin * theta110.cos, phi110.sin * theta110.sin, phi110.cos};
    // glm::dvec3 g111{phi111.sin * theta111.cos, phi111.sin * theta111.sin, phi111.cos};

    // double v000 = glm::dot(p000, g000);
    // double v001 = glm::dot(p001, g001);
    // double v010 = glm::dot(p010, g010);
    // double v011 = glm::dot(p011, g011);
    // double v100 = glm::dot(p100, g100);
    // double v101 = glm::dot(p101, g101);
    // double v110 = glm::dot(p110, g110);
    // double v111 = glm::dot(p111, g111);

    // double u = fade(xt), v = fade(yt), w = fade(zt);

    // double v00 = lerp(u, v000, v100);
    // double v01 = lerp(u, v001, v101);
    // double v10 = lerp(u, v010, v110);
    // double v11 = lerp(u, v011, v111);

    // double v0 = lerp(v, v00, v10);
    // double v1 = lerp(v, v01, v11);

    // return lerp(w, v0, v1);
}

int Perlin::hash2d(int i, int j) const {
    return m_perms_x[i] ^ m_perms_y[j];
}

int Perlin::hash3d(int i, int j, int k) const {
    return m_perms_x[i] ^ m_perms_y[j] ^ m_perms_z[k];
}

double Perlin::fade(double t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

double Perlin::lerp(double t, double a, double b) {
    return t*(b - a) + a;
}

// double Perlin::grad(int hash, double x, double y) {
//     switch (hash & 0x3) {
//         case 0x0: return  x +  y;
//         case 0x1: return -x +  y;
//         case 0x2: return  x + -y;
//         case 0x3: return -x + -y;
//         default : return 0;
//     }
// }

// double Perlin::grad(int hash, double x, double y, double z) {
//     switch (hash & 0xF) {
//         case 0x0: return  x +  y;
//         case 0x1: return -x +  y;
//         case 0x2: return  x + -y;
//         case 0x3: return -x + -y;
//         case 0x4: return  x +  z;
//         case 0x5: return -x +  z;
//         case 0x6: return  x + -z;
//         case 0x7: return -x + -z;
//         case 0x8: return  y +  z;
//         case 0x9: return -y +  z;
//         case 0xA: return  y + -z;
//         case 0xB: return -y + -z;
//         case 0xC: return  x +  y;
//         case 0xD: return -x +  y;
//         case 0xE: return -y +  z;
//         case 0xF: return -y + -z;
//         default: return 0;
//     }
// }

Turbulence::Turbulence(std::shared_ptr<NoiseFunction> base, int depth)
: m_base{base},
  m_depth{depth}
{}

Turbulence::~Turbulence() {}

double Turbulence::operator()(double x, double y) const {
    double accum = 0;
    double weight = 1;

    for (int i = 0; i < m_depth; ++i) {
        accum += weight * (*m_base)(x, y);
        weight *= 0.5;
        x *= 2.0;
        y *= 2.0;
    }

    return accum;
}

double Turbulence::operator()(double x, double y, double z) const {
    double accum = 0;
    double weight = 1;

    for (int i = 0; i < m_depth; ++i) {
        accum += weight * (*m_base)(x, y, z);
        weight *= 0.5;
        x *= 2.0;
        y *= 2.0;
        z *= 2.0;
    }

    return accum;
}

Curve::Curve(std::shared_ptr<NoiseFunction> base, std::shared_ptr<CubicSpline> curve)
    : m_base{base},
      m_curve{curve}
{}

Curve::~Curve() {}

double Curve::operator()(double x, double y) const {
    double rv = (*m_curve)((*m_base)(x, y));
    return rv;
}

double Curve::operator()(double x, double y, double z) const {
    double rv = (*m_curve)((*m_base)(x, y, z));
    return rv;
}
