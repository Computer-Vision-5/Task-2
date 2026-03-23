#include "Snake.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace backend {

namespace {

constexpr float kPi = 3.14159265358979323846f;

inline int clampPx(int v, int bound) {
    return v < 0 ? 0 : (v >= bound ? bound - 1 : v);
}

/// Single-pass 3×3 Gaussian blur to widen the gradient attraction basin.
GrayImage gaussianBlur3x3(const GrayImage& src) {
    static constexpr float k[3][3] = {
        {1/16.f, 2/16.f, 1/16.f},
        {2/16.f, 4/16.f, 2/16.f},
        {1/16.f, 2/16.f, 1/16.f}
    };
    GrayImage out(src.width, src.height, 0.0f);
    for (int y = 0; y < src.height; ++y)
        for (int x = 0; x < src.width; ++x) {
            float s = 0.f;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx)
                    s += k[dy+1][dx+1] * src.at(clampPx(x+dx, src.width),
                                                 clampPx(y+dy, src.height));
            out.at(x, y) = s;
        }
    return out;
}

/// Normalise a vector to [0,1].  If all values are equal, fills with 0.
void normalise(std::vector<float>& v) {
    const float lo = *std::min_element(v.begin(), v.end());
    const float hi = *std::max_element(v.begin(), v.end());
    const float rng = hi - lo;
    if (rng < 1e-8f) { std::fill(v.begin(), v.end(), 0.f); return; }
    for (float& x : v) x = (x - lo) / rng;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Part A — Contour Initialisation
// ─────────────────────────────────────────────────────────────────────────────
SnakeContour initContour(float cx, float cy, float radius, int numPoints)
{
    if (numPoints < 3) numPoints = 3;

    // clamp so no two adjacent points are closer than 1 pixel apart.
    // Circumference of the initial circle is 2*pi*r; we need numPoints <= that.
    // This also ensures arc-length-uniform distribution stays valid at all radii.
    const int maxByCircumference = static_cast<int>(2.0f * kPi * radius);
    if (maxByCircumference >= 3 && numPoints > maxByCircumference)
        numPoints = maxByCircumference;

    SnakeContour c;
    c.reserve(static_cast<size_t>(numPoints));
    const float step = 2.0f * kPi / static_cast<float>(numPoints);
    for (int i = 0; i < numPoints; ++i) {
        float a = static_cast<float>(i) * step;
        c.push_back({ cx + radius * std::cos(a),
                      cy + radius * std::sin(a) });
    }
    return c;
}

// ─────────────────────────────────────────────────────────────────────────────
// Part B — Greedy Energy-Minimisation
//
// Key design: all three energy terms are normalised to [0,1] PER POINT PER
// ITERATION before being combined with α and β.  Without this the gradient²
// term (potentially tens-of-thousands) overwhelms the contour terms and α/β
// become meaningless — causing the snake to chase the globally strongest edge
// rather than the nearest relevant one.
// ─────────────────────────────────────────────────────────────────────────────
SnakeContour evolveContour(const SnakeContour&  initial,
                           const GrayImage&      gradientMagnitude,
                           const SnakeParams&    params)
{
    if (initial.empty() || !gradientMagnitude.isValid())
        return initial;

    const GrayImage smoothGrad = gaussianBlur3x3(gradientMagnitude);

    const int   W    = params.windowSize;
    const float alpha= params.alpha;
    const float beta = params.beta;
    const int   imgW = gradientMagnitude.width;
    const int   imgH = gradientMagnitude.height;
    const int   N    = static_cast<int>(initial.size());

    // Reference average inter-point spacing — fixed from the INITIAL contour
    // so Eelastic always penalises deviation from the original circle spacing,
    // not the current (possibly collapsed) contour spacing.
    //Computes the average arc length between adjacent points on the initial circle 
    float refDist = 0.0f;
    for (int i = 0; i < N; ++i) {
        const int prev = (i - 1 + N) % N;
        const float dx = initial[i][0] - initial[prev][0];
        const float dy = initial[i][1] - initial[prev][1];
        refDist += std::sqrt(dx*dx + dy*dy);
    }
    refDist /= static_cast<float>(N);

    SnakeContour contour = initial;
    //Pre-compute the search window dimensions. If W=5, the window is 11×11 = 121 candidate positions per point per iteration
    const int winSide = 2*W + 1;
    const int winArea = winSide * winSide;

    for (int iter = 0; iter < params.maxIterations; ++iter) {
        // accumulates total movement to check convergence.
        float totalDisp = 0.0f;
        SnakeContour next = contour; //next is a copy of the current contour that will hold the updated positions after this iteration.

        for (int i = 0; i < N; ++i) {
            const int prev = (i - 1 + N) % N;
            const int nxt  = (i + 1)     % N;

            const float px = contour[prev][0], py = contour[prev][1];
            const float cx = contour[i][0],    cy = contour[i][1];
            const float nx = contour[nxt][0],  ny = contour[nxt][1];

            //Round the floating-point position to the nearest integer pixel — this is the centre of the search window.
            const int cx_int = static_cast<int>(std::round(cx));
            const int cy_int = static_cast<int>(std::round(cy));

            // ── Collect raw energies for every candidate in the window ──────
            std::vector<float> eImg(static_cast<size_t>(winArea));
            std::vector<float> eEla(static_cast<size_t>(winArea));
            std::vector<float> eSmth(static_cast<size_t>(winArea));
            // Candidate coordinates
            std::vector<float> candX(static_cast<size_t>(winArea));
            std::vector<float> candY(static_cast<size_t>(winArea));

            int validCount = 0;
            for (int dy = -W; dy <= W; ++dy) {
                for (int dx = -W; dx <= W; ++dx) {
                    const int sx = cx_int + dx;
                    const int sy = cy_int + dy;

                    // Skip out-of-image candidates entirely
                    if (sx < 0 || sy < 0 || sx >= imgW || sy >= imgH)
                        continue;

                    const float fx = static_cast<float>(sx);
                    const float fy = static_cast<float>(sy);
                    const size_t idx = static_cast<size_t>(validCount);

                    candX[idx] = fx;
                    candY[idx] = fy;

                    // Eimage = -(blurred_gradient)²  (negative → attract to edges)
                    const float g = smoothGrad.at(sx, sy);
                    eImg[idx] = -(g * g);

                    // Eelastic: (distance to previous point − reference)²
                    const float ex = fx - px, ey = fy - py;
                    const float d  = std::sqrt(ex*ex + ey*ey);
                    const float dd = d - refDist;
                    eEla[idx] = dd * dd;

                    // Esmooth: second-difference curvature magnitude²
                    const float scx = px - 2.f*fx + nx;
                    const float scy = py - 2.f*fy + ny;
                    eSmth[idx] = scx*scx + scy*scy;

                    ++validCount;
                }
            }

            if (validCount == 0) continue;  // entire window out of image → skip

            // Resize to actual valid count
            eImg .resize(static_cast<size_t>(validCount));
            eEla .resize(static_cast<size_t>(validCount));
            eSmth.resize(static_cast<size_t>(validCount));
            candX.resize(static_cast<size_t>(validCount));
            candY.resize(static_cast<size_t>(validCount));

            // ── Normalise each term independently to [0,1] ─────────────────
            normalise(eImg);
            normalise(eEla);
            normalise(eSmth);

            // ── Find minimum combined energy ────────────────────────────────
            float bestE = std::numeric_limits<float>::max();
            float bestX = cx, bestY = cy;

            for (int k = 0; k < validCount; ++k) {
                const float E = eImg[static_cast<size_t>(k)]
                              + alpha * eEla [static_cast<size_t>(k)]
                              + beta  * eSmth[static_cast<size_t>(k)];
                if (E < bestE) {
                    bestE = E;
                    bestX = candX[static_cast<size_t>(k)];
                    bestY = candY[static_cast<size_t>(k)];
                }
            }

            const float ddx = bestX - cx;
            const float ddy = bestY - cy;
            totalDisp += std::sqrt(ddx*ddx + ddy*ddy);
            next[static_cast<size_t>(i)] = {bestX, bestY};
        }

        contour = next;

        if (totalDisp < params.convergenceThreshold)
            break;
    }

    return contour;
}

// ─────────────────────────────────────────────────────────────────────────────
// Part C — Chain-Code Representation (Freeman 8-connectivity)
//
// Each contour point is rounded to the nearest integer pixel.  The direction
// vector (dx, dy) to the *next* point (with wrap-around) is then quantised
// to one of the 8 Freeman directions:
//
//   3  2  1
//   4  *  0   (0 = East, counter-clockwise increments)
//   5  6  7
//
// If two adjacent points round to the same pixel the code defaults to 0.
// ─────────────────────────────────────────────────────────────────────────────
std::vector<int> contourChainCode(const SnakeContour& contour)
{
    const int N = static_cast<int>(contour.size());
    std::vector<int> codes(static_cast<size_t>(N), 0);
    if (N < 2) return codes;

    for (int i = 0; i < N; ++i) {
        const int next = (i + 1) % N;

        // Round to nearest integer pixel
        const int x0 = static_cast<int>(std::round(contour[static_cast<size_t>(i)   ][0]));
        const int y0 = static_cast<int>(std::round(contour[static_cast<size_t>(i)   ][1]));
        const int x1 = static_cast<int>(std::round(contour[static_cast<size_t>(next)][0]));
        const int y1 = static_cast<int>(std::round(contour[static_cast<size_t>(next)][1]));

        const int dx = x1 - x0;
        const int dy = y1 - y0;

        // Map (dx, dy) to Freeman 8-direction code.
        // We normalise to the sign pair to handle steps > 1 pixel.
        const int sx = (dx > 0) ? 1 : (dx < 0) ? -1 : 0;
        const int sy = (dy > 0) ? 1 : (dy < 0) ? -1 : 0;

        // Lookup table: rows indexed by (sy+1), columns by (sx+1)
        //        sx: -1   0  +1
        // sy: -1      3   2   1
        // sy:  0      4   0   0
        // sy: +1      5   6   7
        static constexpr int lut[3][3] = {
            { 3, 2, 1 },  // sy == -1  (North in image coords)
            { 4, 0, 0 },  // sy ==  0
            { 5, 6, 7 }   // sy == +1  (South in image coords)
        };

        codes[static_cast<size_t>(i)] = lut[sy + 1][sx + 1];
    }
    return codes;
}

// ─────────────────────────────────────────────────────────────────────────────
// Part D — Enclosed Area (Shoelace / Gauss formula)
//
// A = |Σ_{i=0}^{N-1} (x_i * y_{i+1} - x_{i+1} * y_i)| / 2
// where indices are taken mod N (closed polygon).
// ─────────────────────────────────────────────────────────────────────────────
float contourArea(const SnakeContour& contour)
{
    const int N = static_cast<int>(contour.size());
    if (N < 3) return 0.0f;

    float sum = 0.0f;
    for (int i = 0; i < N; ++i) {
        const int j = (i + 1) % N;
        const float xi = contour[static_cast<size_t>(i)][0];
        const float yi = contour[static_cast<size_t>(i)][1];
        const float xj = contour[static_cast<size_t>(j)][0];
        const float yj = contour[static_cast<size_t>(j)][1];
        sum += xi * yj - xj * yi;
    }
    return std::abs(sum) * 0.5f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Part E — Perimeter (sum of Euclidean chord lengths)
//
// P = Σ_{i=0}^{N-1} || p_{i+1} - p_i ||
// where the last edge wraps from p_{N-1} back to p_0.
// ─────────────────────────────────────────────────────────────────────────────
float contourPerimeter(const SnakeContour& contour)
{
    const int N = static_cast<int>(contour.size());
    if (N < 2) return 0.0f;

    float total = 0.0f;
    for (int i = 0; i < N; ++i) {
        const int j = (i + 1) % N;
        const float dx = contour[static_cast<size_t>(j)][0] - contour[static_cast<size_t>(i)][0];
        const float dy = contour[static_cast<size_t>(j)][1] - contour[static_cast<size_t>(i)][1];
        total += std::sqrt(dx * dx + dy * dy);
    }
    return total;
}

} // namespace backend
