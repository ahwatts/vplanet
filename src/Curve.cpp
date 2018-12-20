// -*- mode: c++; c-basic-offset: 4; indent-tabs-mode: nil; -*-

#include <algorithm>
#include <limits>

#include "Curve.h"

CubicSpline::CubicSpline()
    : m_cps{},
      m_coeffs{}
{}

CubicSpline::~CubicSpline() {}

CubicSpline& CubicSpline::addControlPoint(double x, double y) {
    double epsilon = std::numeric_limits<double>::epsilon();
    for (auto cp : m_cps) {
        if (std::abs(cp.first - x) < epsilon) {
            return *this;
        }
    }

    auto insert_before = m_cps.cbegin();
    while (insert_before != m_cps.cend()) {
        if (insert_before->first > x) {
            break;
        }
        ++insert_before;
    }

    if (insert_before == m_cps.cend()) {
        m_cps.push_back({ x, y });
    } else {
        m_cps.insert(insert_before, { x, y });
    }

    if (m_cps.size() >= 2) {
        generateCoeffs();
    }

    return *this;
}

double CubicSpline::operator()(double x) const {
    if (x < m_cps.front().first) {
        return m_cps.front().second;
    }

    if (x > m_cps.back().first) {
        return m_cps.back().second;
    }

    int i, n = static_cast<int>(m_cps.size()) - 1;
    for (i = n - 1; i >= 0; --i) {
        if (x - m_cps[i].first >= 0) {
            break;
        }
    }

    double alpha = x - m_cps[i].first;
    double h = m_cps[i+1].first - m_cps[i].first;
    double rv = 0.5*m_coeffs[i] + alpha*(m_coeffs[i+1] - m_coeffs[i])/(6*h);
    rv = -1*(h/6.0)*(m_coeffs[i+1] + 2*m_coeffs[i]) + (m_cps[i+1].second - m_cps[i].second)/h + alpha*rv;
    return m_cps[i].second + alpha*rv;
}

void CubicSpline::generateCoeffs() {
    int n = static_cast<int>(m_cps.size()) - 1;
    m_coeffs.clear();
    m_coeffs.resize(n+1);
    std::vector<double> h(n), b(n), u(n), v(n);

    for (int i = 0; i < n; ++i) {
        h[i] = m_cps[i+1].first - m_cps[i].first;
        b[i] = (m_cps[i+1].second - m_cps[i].second) / h[i];
    }

    if (n > 1) {
        u[1] = 2*(h[0] + h[1]);
        v[1] = 6*(b[1] - b[0]);

        for (int i = 2; i < n; ++i) {
            u[i] = 2*(h[i] + h[i-1]) - h[i-1]*h[i-1]/u[i-1];
            v[i] = 6*(b[i] - b[i-1]) - h[i-1]*v[i-1]/u[i-1];
        }
    }

    m_coeffs[n] = 0;

    if (n > 1) {
        for (int i = n-1; i > 0; --i) {
            m_coeffs[i] = (v[i] - h[i]*m_coeffs[i+1]) / u[i];
        }
    }

    m_coeffs[0] = 0;
}

/*
CurveDisplay CurveDisplay::createCurveDisplay(CubicSpline &curve, double min_x, double max_x, double min_y, double max_y, int num_points) {
    CurveDisplay rv;

    std::vector<glm::vec2> positions;
    std::vector<unsigned int> elements;
    double domain = max_x - min_x;
    double range = max_y - min_y;
    double h = domain / (num_points - 1);
    for (int i = 0; i < num_points; ++i) {
        double x = min_x + i*h;
        double y = curve(x);

        double clip_x = 1.8*(x - min_x)/domain - 0.9;
        double clip_y = 0.9*(y - min_y)/range - 0.9;

        // std::cout << "(" << x << ", " << y << ") -> (" << clip_x << ", " << clip_y << ")" << std::endl;

        elements.push_back(static_cast<unsigned int>(positions.size()));
        positions.push_back({ clip_x, clip_y });
    }

    rv.createBuffers(positions, elements);
    rv.createProgram();
    rv.createArrayObject();

    return rv;
}

CurveDisplay::CurveDisplay()
    : m_array_buffer{0},
      m_elem_buffer{0},
      m_program{0},
      m_array_object{0},
      m_num_elems{0},
      m_position_loc{0},
      m_color_loc{0}
{}

CurveDisplay::~CurveDisplay() {
    std::vector<GLuint> bufs_to_delete;

    if (glIsBuffer(m_array_buffer)) {
        bufs_to_delete.push_back(m_array_buffer);
    }

    if (glIsBuffer(m_elem_buffer)) {
        bufs_to_delete.push_back(m_elem_buffer);
    }

    if (bufs_to_delete.size() > 0) {
        glDeleteBuffers(static_cast<GLsizei>(bufs_to_delete.size()), bufs_to_delete.data());
    }

    m_array_buffer = 0;
    m_elem_buffer = 0;

    if (glIsProgram(m_program)) {
        glDeleteProgram(m_program);
    }

    m_program = 0;

    if (glIsVertexArray(m_array_object)) {
        glDeleteVertexArrays(1, &m_array_object);
    }

    m_array_object = 0;
}

void CurveDisplay::render() const {
    glUseProgram(m_program);

    glDisable(GL_DEPTH_TEST);
    glLineWidth(2.0);
    glBindVertexArray(m_array_object);
    glDrawElements(GL_LINE_STRIP, m_num_elems, GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);
    glUseProgram(0);
}

void CurveDisplay::createBuffers(const std::vector<glm::vec2> &vertices, const std::vector<unsigned int> &elems) {
    GLuint buffers[2];
    glGenBuffers(2, buffers);
    m_array_buffer = buffers[0];
    m_elem_buffer = buffers[1];

    glBindBuffer(GL_ARRAY_BUFFER, m_array_buffer);
    glBufferData(
        GL_ARRAY_BUFFER,
        vertices.size()*sizeof(glm::vec2),
        vertices.data(),
        GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elem_buffer);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        elems.size()*sizeof(unsigned int),
        elems.data(),
        GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    m_num_elems = static_cast<unsigned int>(elems.size());
}

void CurveDisplay::createProgram() {
    Resource vert_code = LOAD_RESOURCE(curve_vert);
    Resource frag_code = LOAD_RESOURCE(curve_frag);

    GLuint vert_shader = createAndCompileShader(GL_VERTEX_SHADER, vert_code.toString().data());
    GLuint frag_shader = createAndCompileShader(GL_FRAGMENT_SHADER, frag_code.toString().data());
    m_program = createProgramFromShaders(vert_shader, frag_shader);
    m_position_loc = glGetAttribLocation(m_program, "position");

    glDeleteShader(vert_shader);
    glDeleteShader(frag_shader);
}

void CurveDisplay::createArrayObject() {
    glGenVertexArrays(1, &m_array_object);
    glUseProgram(m_program);
    glBindVertexArray(m_array_object);
    glBindBuffer(GL_ARRAY_BUFFER, m_array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_elem_buffer);

    glEnableVertexAttribArray(m_position_loc);
    glVertexAttribPointer(
        m_position_loc,
        2, GL_FLOAT, GL_FALSE,
        sizeof(glm::vec2),
        (const void *)(0)
    );

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
*/