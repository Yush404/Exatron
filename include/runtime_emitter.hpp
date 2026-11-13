#pragma once
#include <string_view>

namespace exatron {

inline constexpr std::string_view EXATRON_RUNTIME_HEADER = R"RUNTIME(
// Exatron Runtime (auto-generated)
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <iomanip>
#include <cstdint>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <cstdlib>

namespace exatron_rt {

inline uint32_t g_rand_state = 1337;

inline int64_t idiv(int64_t a, int64_t b) {
    if (b == 0) {
        std::cerr << "Runtime Error: division by zero\n";
        std::exit(1);
    }
    return a / b;
}

inline int64_t imod(int64_t a, int64_t b) {
    if (b == 0) {
        std::cerr << "Runtime Error: division by zero\n";
        std::exit(1);
    }
    return a % b;
}

struct Tensor {
    int rows;
    int cols;
    std::vector<float> data;

    Tensor(int r, int c, float fill = 0.0f)
        : rows(r), cols(c), data(static_cast<std::size_t>(r * c), fill) {}

    Tensor(int r, int c, std::initializer_list<float> vals)
        : rows(r), cols(c), data(vals) {
        data.resize(static_cast<std::size_t>(r * c), 0.0f);
    }

    float& at(int r, int c) { return data[static_cast<std::size_t>(r * cols + c)]; }
    float at(int r, int c) const { return data[static_cast<std::size_t>(r * cols + c)]; }

    static Tensor zeros(int r, int c) {
        return Tensor(r, c, 0.0f);
    }

    static Tensor ones(int r, int c) {
        return Tensor(r, c, 1.0f);
    }

    static Tensor rand(int r, int c) {
        Tensor t(r, c);
        for (int i = 0; i < r * c; ++i) {
            g_rand_state = g_rand_state * 1664525u + 1013904223u;
            float v = static_cast<float>(g_rand_state >> 8) / 16777216.0f - 0.5f;
            t.data[static_cast<std::size_t>(i)] = v;
        }
        return t;
    }
};

inline Tensor operator+(const Tensor& a, const Tensor& b) {
    Tensor out(a.rows, a.cols);
    for (std::size_t i = 0; i < a.data.size(); ++i)
        out.data[i] = a.data[i] + b.data[i];
    return out;
}

inline Tensor operator-(const Tensor& a, const Tensor& b) {
    Tensor out(a.rows, a.cols);
    for (std::size_t i = 0; i < a.data.size(); ++i)
        out.data[i] = a.data[i] - b.data[i];
    return out;
}

inline Tensor operator*(const Tensor& a, const Tensor& b) {
    Tensor out(a.rows, a.cols);
    for (std::size_t i = 0; i < a.data.size(); ++i)
        out.data[i] = a.data[i] * b.data[i];
    return out;
}

inline Tensor add_bias(const Tensor& x, const Tensor& bias_1xN) {
    Tensor out = x;
    for (int i = 0; i < out.rows; ++i)
        for (int j = 0; j < out.cols; ++j)
            out.at(i, j) += bias_1xN.at(0, j);
    return out;
}

inline Tensor matmul(const Tensor& a, const Tensor& b) {
    Tensor out(a.rows, b.cols, 0.0f);
    for (int i = 0; i < a.rows; ++i) {
        for (int k = 0; k < a.cols; ++k) {
            float aik = a.at(i, k);
            for (int j = 0; j < b.cols; ++j) {
                out.at(i, j) += aik * b.at(k, j);
            }
        }
    }
    return out;
}

// ── Activation Functions ────────────────────────────────────────────────────

inline Tensor relu(const Tensor& x) {
    Tensor out = x;
    for (auto& v : out.data) v = std::max(0.0f, v);
    return out;
}

inline Tensor sigmoid(const Tensor& x) {
    Tensor out = x;
    for (auto& v : out.data) v = 1.0f / (1.0f + std::exp(-v));
    return out;
}

inline Tensor tanh_act(const Tensor& x) {
    Tensor out = x;
    for (auto& v : out.data) v = std::tanh(v);
    return out;
}

inline Tensor softmax(const Tensor& x) {
    Tensor out = x;
    for (int i = 0; i < out.rows; ++i) {
        float max_val = out.at(i, 0);
        for (int j = 1; j < out.cols; ++j)
            max_val = std::max(max_val, out.at(i, j));
        float sum = 0.0f;
        for (int j = 0; j < out.cols; ++j) {
            out.at(i, j) = std::exp(out.at(i, j) - max_val);
            sum += out.at(i, j);
        }
        for (int j = 0; j < out.cols; ++j)
            out.at(i, j) /= sum;
    }
    return out;
}

inline Tensor linear(const Tensor& x) {
    return x;
}

// ── DenseLayer ──────────────────────────────────────────────────────────────

struct DenseLayer {
    Tensor weights;
    Tensor bias;
    DenseLayer(int in_f, int out_f)
        : weights(Tensor::rand(in_f, out_f)), bias(Tensor::zeros(1, out_f)) {}
    Tensor linear_step(const Tensor& input) const {
        return add_bias(matmul(input, weights), bias);
    }
};

// Print Helpers

inline void print_val(int64_t v) { std::cout << v << "\n"; }
inline void print_val(float v) { std::cout << v << "\n"; }
inline void print_val(bool v) { std::cout << (v ? "true" : "false") << "\n"; }
inline void print_val(const std::string& v) { std::cout << v << "\n"; }
inline void print_val(const char* v) { print_val(std::string(v)); }
inline void print_val(const Tensor& t) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(4);
    ss << "[";
    for (int i = 0; i < t.rows; ++i) {
        if (i > 0) ss << ", ";
        ss << "[";
        for (int j = 0; j < t.cols; ++j) {
            if (j > 0) ss << ", ";
            ss << t.at(i, j);
        }
        ss << "]";
    }
    ss << "]\n";
    std::cout << ss.str();
}

} // namespace exatron_rt
)RUNTIME";

} // namespace exatron

