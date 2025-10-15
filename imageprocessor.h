#pragma once

#include <QImage>

namespace Img
{
// Преобразование изображения в оттенки серого (8 бит на пиксель)
QImage toGrayscale(const QImage& src);

// ---------------- ГЛОБАЛЬНЫЕ ПОРОГОВЫЕ МЕТОДЫ ----------------

// Метод Отсу (автоматический выбор порога)
QImage thresholdOtsu(const QImage& gray);

// Итеративный метод (ISODATA)
QImage thresholdIterative(const QImage& gray);

QImage thresholdMean(const QImage& gray);

//-----------------АДАПТИВНАЯ ПОРОГОВАЯ ОБРАБОТКА-------------
// α-метод по лекции: порог t = α * φ(local min/max/mean) и сравнение |f - Ĥ| > t,
// где Ĥ — локальная оценка (используем локальное среднее).
QImage adaptiveAlpha(const QImage& gray, int windowSize, double alpha);

// ---------------- СЕГМЕНТАЦИЯ ----------------

// Перепады яркости (границы) — оператор Собеля, модуль градиента + порог
QImage edgesSobel(const QImage& gray, int threshold);

// Обнаружение линий — направленные ядра (горизонтальные, вертикальные, диагональные),
// берём максимум по модулям откликов и порог
QImage linesKernels(const QImage& gray, int threshold);

// Обнаружение точек — Лапласиан (по модулю) + порог
QImage pointsLaplacian(const QImage& gray, int threshold);

} // namespace Img
