#include "ImageProcessor.h"
#include <QtMath>
#include <vector>
#include <algorithm>
#include <cstdint>

namespace {

// Доступ к пикселям как uchar
inline const uchar* rowPtr(const QImage& im, int y) { return im.constScanLine(y); }
inline uchar*       rowPtr(QImage& im, int y)       { return im.scanLine(y); }

// Отражение индекса для граничных условий (reflect)
inline int refl(int i, int n) {
    if (i < 0)   return -i - 1;
    if (i >= n)  return 2*n - i - 1;
    return i;
}

// Получить значение пикселя с отражением
inline int getPixReflect(const QImage& g, int x, int y) {
    const int xx = refl(x, g.width());
    const int yy = refl(y, g.height());
    return rowPtr(g, yy)[xx];
}

// Единственная «атомарная» signed-свёртка в точке (x,y)
template<typename T>
int convAtSigned(const QImage& g, const std::vector<T>& k, int kw, int kh, int x, int y)
{
    const int cx = kw/2;
    const int cy = kh/2;
    long long acc = 0;
    for (int j=0; j<kh; ++j) {
        for (int i=0; i<kw; ++i) {
            const int px = getPixReflect(g, x + i - cx, y + j - cy);
            acc += 1LL * px * k[j*kw + i];
        }
    }
    return static_cast<int>(acc);
}

// Интегральное изображение (для быстрого локального среднего)
void integralImageU8(const QImage& g, std::vector<uint32_t>& ii)
{
    const int w = g.width(), h = g.height();
    ii.assign((w+1)*(h+1), 0);
    for (int y=1; y<=h; ++y) {
        uint32_t rowSum = 0;
        const uchar* p = rowPtr(g, y-1);
        for (int x=1; x<=w; ++x) {
            rowSum += p[x-1];
            ii[y*(w+1)+x] = ii[(y-1)*(w+1)+x] + rowSum;
        }
    }
}
inline uint32_t rectSum(const std::vector<uint32_t>& ii, int w, int x0,int y0,int x1,int y1) {
    // прямоугольник [x0,x1) x [y0,y1)
    return ii[y1*(w+1)+x1] - ii[y0*(w+1)+x1] - ii[y1*(w+1)+x0] + ii[y0*(w+1)+x0];
}

} // namespace

namespace Img {

QImage toGrayscale(const QImage& src)
{
    if (src.format() == QImage::Format_Grayscale8) return src;
    return src.convertToFormat(QImage::Format_Grayscale8);
}

// ---------------- Глобальные пороги ----------------

QImage thresholdOtsu(const QImage& gray)
{
    const int w = gray.width();
    const int h = gray.height();
    const int L = 256;

    int hist[L] = {0};
    for (int y = 0; y < h; ++y) {
        const uchar* p = rowPtr(gray, y);
        for (int x = 0; x < w; ++x) hist[p[x]]++;
    }

    const double H = double(w * h);

    double p[L];
    for (int i = 0; i < L; ++i) p[i] = hist[i] / H;

    double muT = 0.0;
    for (int i = 0; i < L; ++i) muT += i * p[i];

    double P0 = 0.0, mu0sum = 0.0;
    double maxSigmaB = 0.0;
    int bestT = 0;

    for (int t = 0; t < L; ++t) {
        P0 += p[t];
        mu0sum += t * p[t];

        if (P0 <= 0.0 || P0 >= 1.0) continue;

        const double mu0 = mu0sum / P0;
        const double mu1 = (muT - mu0sum) / (1.0 - P0);
        const double sigmaB2 = P0*(1.0 - P0)*(mu0 - mu1)*(mu0 - mu1);
        if (sigmaB2 > maxSigmaB) { maxSigmaB = sigmaB2; bestT = t; }
    }

    QImage out(w, h, QImage::Format_Grayscale8);
    for (int y=0; y<h; ++y) {
        const uchar* s = rowPtr(gray, y);
        uchar* d = rowPtr(out, y);
        for (int x=0; x<w; ++x) d[x] = (s[x] > bestT) ? 255 : 0;
    }
    return out;
}

QImage thresholdIterative(const QImage& gray)
{
    const int w = gray.width(), h = gray.height();
    long long sum = 0;
    for (int y=0; y<h; ++y) {
        const uchar* p = rowPtr(gray, y);
        for (int x=0; x<w; ++x) sum += p[x];
    }
    double T = double(sum) / (w*h);

    for (int iter=0; iter<50; ++iter) {
        double m1=0, m2=0; int c1=0, c2=0;
        for (int y=0; y<h; ++y) {
            const uchar* p = rowPtr(gray, y);
            for (int x=0; x<w; ++x) {
                if (p[x] <= T) { m1 += p[x]; ++c1; }
                else           { m2 += p[x]; ++c2; }
            }
        }
        if (c1==0 || c2==0) break;
        const double newT = 0.5 * (m1/c1 + m2/c2);
        if (qFabs(newT - T) < 0.5) { T = newT; break; }
        T = newT;
    }

    QImage out(w, h, QImage::Format_Grayscale8);
    for (int y=0; y<h; ++y) {
        const uchar* s = rowPtr(gray, y);
        uchar* d = rowPtr(out, y);
        for (int x=0; x<w; ++x) d[x] = (s[x] > T) ? 255 : 0;
    }
    return out;
}

QImage thresholdMean(const QImage& gray)
{
    const int w = gray.width();
    const int h = gray.height();
    long long sum = 0;

    for (int y = 0; y < h; ++y) {
        const uchar* p = gray.constScanLine(y);
        for (int x = 0; x < w; ++x)
            sum += p[x];
    }

    double mean = double(sum) / (w * h);

    QImage out(w, h, QImage::Format_Grayscale8);
    for (int y = 0; y < h; ++y) {
        const uchar* p = gray.constScanLine(y);
        uchar* o = out.scanLine(y);
        for (int x = 0; x < w; ++x)
            o[x] = (p[x] > mean) ? 255 : 0;
    }

    return out;
}



// ---------------- Сегментация ----------------

QImage edgesSobel(const QImage& gray, int threshold)
{
    static const int kx[9] = { -1,0,1, -2,0,2, -1,0,1 };
    static const int ky[9] = { -1,-2,-1, 0,0,0, 1,2,1 };
    const std::vector<int> Kx(kx, kx+9);
    const std::vector<int> Ky(ky, ky+9);

    const int w = gray.width(), h = gray.height();
    QImage out(w, h, QImage::Format_Grayscale8);

    for (int y=0; y<h; ++y) {
        uchar* d = rowPtr(out, y);
        for (int x=0; x<w; ++x) {
            const int gx = convAtSigned(gray, Kx, 3, 3, x, y);
            const int gy = convAtSigned(gray, Ky, 3, 3, x, y);
            const int mag = int(std::hypot(double(gx), double(gy)));
            d[x] = (mag >= threshold) ? 255 : 0;
        }
    }
    return out;
}

QImage linesKernels(const QImage& gray, int threshold)
{
    // 3x3 ядра: гориз., верт., диагонали
    static const int kh[9] = { -1,-1,-1,  2,2,2,  -1,-1,-1 };
    static const int kv[9] = { -1, 2,-1, -1,2,-1, -1, 2,-1 };
    static const int kd1[9]= {  2,-1,-1, -1,2,-1, -1,-1, 2 };
    static const int kd2[9]= { -1,-1, 2, -1,2,-1,  2,-1,-1 };

    const std::vector<int> Kh(kh, kh+9), Kv(kv, kv+9), Kd1(kd1, kd1+9), Kd2(kd2, kd2+9);

    const int w = gray.width(), h = gray.height();
    QImage out(w, h, QImage::Format_Grayscale8);

    for (int y=0; y<h; ++y) {
        uchar* d = rowPtr(out, y);
        for (int x=0; x<w; ++x) {
            const int rh  = convAtSigned(gray, Kh,  3,3, x,y);
            const int rv  = convAtSigned(gray, Kv,  3,3, x,y);
            const int r45 = convAtSigned(gray, Kd1, 3,3, x,y);
            const int r135= convAtSigned(gray, Kd2, 3,3, x,y);
            const int m = std::max({ std::abs(rh), std::abs(rv), std::abs(r45), std::abs(r135) });
            d[x] = (m >= threshold) ? 255 : 0;
        }
    }
    return out;
}

QImage pointsLaplacian(const QImage& gray, int threshold)
{
    // Классический 3x3 Лаплас (8-соседство)
    static const int lap[9] = { -1,-1,-1, -1,8,-1, -1,-1,-1 };
    const std::vector<int> K(lap, lap+9);

    const int w = gray.width(), h = gray.height();
    QImage out(w, h, QImage::Format_Grayscale8);

    for (int y=0; y<h; ++y) {
        uchar* d = rowPtr(out, y);
        for (int x=0; x<w; ++x) {
            const int r = convAtSigned(gray, K, 3,3, x,y);
            d[x] = (std::abs(r) >= threshold) ? 255 : 0;
        }
    }
    return out;
}

// ---------------- Адаптивная пороговая обработка (α-метод) ----------------

QImage adaptiveAlpha(const QImage& gray, int windowSize, double alpha)
{
    const int w = gray.width();
    const int h = gray.height();
    if (w==0 || h==0) return QImage();

    // делаем окно нечётным и >=3
    if (windowSize < 3) windowSize = 3;
    if ((windowSize & 1) == 0) ++windowSize;
    const int K = windowSize/2;

    // Интегральное изображение для быстрого локального среднего
    std::vector<uint32_t> ii;
    integralImageU8(gray, ii);

    QImage out(w, h, QImage::Format_Grayscale8);

    for (int y=0; y<h; ++y) {
        const uchar* src = rowPtr(gray, y);
        uchar* dst = rowPtr(out, y);

        for (int x=0; x<w; ++x) {
            // 1) локальная область (обрезка по границам, не reflect)
            const int x0 = std::max(0, x - K);
            const int x1 = std::min(w - 1, x + K);
            const int y0 = std::max(0, y - K);
            const int y1 = std::min(h - 1, y + K);

            const int rx0 = x0, ry0 = y0, rx1 = x1 + 1, ry1 = y1 + 1;
            const int count = (x1 - x0 + 1) * (y1 - y0 + 1);
            const double mean = double(rectSum(ii, w, rx0, ry0, rx1, ry1)) / double(count);

            // 2) локальные fmin/fmax (пробегаем окно — достаточно быстро для учебной задачи)
            int fmin = 255, fmax = 0;
            for (int yy=y0; yy<=y1; ++yy) {
                const uchar* p = rowPtr(gray, yy);
                for (int xx=x0; xx<=x1; ++xx) {
                    const int v = p[xx];
                    if (v < fmin) fmin = v;
                    if (v > fmax) fmax = v;
                }
            }

            const double dFmax = std::abs(fmax - mean);
            const double dFmin = std::abs(fmin - mean);

            // 3) выбор порога t по лекционной логике
            double t = 0.0;
            if (fmax == fmin) {
                t = alpha * mean;
            } else if (std::abs(dFmax - dFmin) < 1e-9) {
                // равновесие — берём α*mean
                t = alpha * mean;
            } else if (dFmax > dFmin) {
                t = alpha * ( (2.0/3.0)*fmin + (1.0/3.0)*mean );
            } else {
                t = alpha * ( (1.0/3.0)*fmin + (2.0/3.0)*mean );
            }

            // 4) сравнение |f - Ĥ| > t, где Ĥ ≡ mean
            const double Hxy = mean;
            dst[x] = (std::abs(int(src[x]) - Hxy) > t) ? 255 : 0;
        }
    }

    return out;
}

} // namespace Img
