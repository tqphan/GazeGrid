#pragma once
#include <QList>
#include <QRectF>
#include <QRect>
#include <QSize>
#include <QPointF>
#include <cmath>

// All display rectangles are in compositor logical coordinates, not native pixels.
namespace DisplayMapping {
struct Mapping {
    int region = -1;
    QRectF target;
    bool valid() const { return region >= 0 && target.isValid(); }
    QPointF point(QPointF normalized) const {
        if (!valid() || !std::isfinite(normalized.x()) || !std::isfinite(normalized.y()) ||
            normalized.x() < 0 || normalized.x() > 1 || normalized.y() < 0 || normalized.y() > 1) return {-1, -1};
        return {target.x() + normalized.x() * qMax(0.0, target.width() - 1),
                target.y() + normalized.y() * qMax(0.0, target.height() - 1)};
    }
};
inline QRectF bounds(const QList<QRectF> &rects) {
    QRectF result;
    for (const auto &rect : rects) {
        if (!rect.isValid() || !std::isfinite(rect.x()) || !std::isfinite(rect.y()) ||
            !std::isfinite(rect.width()) || !std::isfinite(rect.height())) return {};
        result = result.isValid() ? result.united(rect) : rect;
    }
    return result;
}
inline bool sameRect(QRectF a, QRectF b) {
    return qAbs(a.x()-b.x()) <= 1 && qAbs(a.y()-b.y()) <= 1 &&
           qAbs(a.width()-b.width()) <= 1 && qAbs(a.height()-b.height()) <= 1;
}
inline Mapping input(const QList<QRectF> &screens, int target, const QList<QRectF> &regions) {
    if (target < 0 || target >= screens.size() || regions.isEmpty()) return {};
    const auto desktop = bounds(screens), inputBounds = bounds(regions);
    if (!desktop.isValid() || !inputBounds.isValid()) return {};
    // A single display/region retains the original normalized-coordinate behavior.
    if (screens.size() == 1 && regions.size() == 1) return {0, regions.front()};
    const double sx = inputBounds.width()/desktop.width(), sy = inputBounds.height()/desktop.height();
    if (qAbs(sx-sy) * qMax(desktop.width(), desktop.height()) > 1) return {};
    auto transform = [&](QRectF rect) {
        return QRectF(inputBounds.x()+(rect.x()-desktop.x())*sx,
                      inputBounds.y()+(rect.y()-desktop.y())*sy, rect.width()*sx, rect.height()*sy);
    };
    // Some compositors provide one region covering the whole virtual desktop.
    if (regions.size() == 1) return {0, transform(screens[target])};
    if (regions.size() != screens.size()) return {};
    QList<int> used;
    Mapping selected;
    for (int screen = 0; screen < screens.size(); ++screen) {
        const auto expected = transform(screens[screen]);
        int match = -1;
        for (int region = 0; region < regions.size(); ++region) {
            if (!sameRect(expected, regions[region])) continue;
            if (match >= 0) return {}; // Never choose between ambiguous regions.
            match = region;
        }
        if (match < 0 || used.contains(match)) return {};
        used << match;
        if (screen == target) selected = {match, regions[match]};
    }
    return selected;
}
inline QRect screenshot(const QList<QRectF> &screens, int target, QSize pixels) {
    if (target < 0 || target >= screens.size() || !pixels.isValid()) return {};
    const auto desktop = bounds(screens);
    if (!desktop.isValid()) return {};
    const double sx = pixels.width()/desktop.width(), sy = pixels.height()/desktop.height();
    // Full-workspace portal captures use a common raster scale, including mixed-DPI
    // KDE workspaces. Reject unrelated single-window/single-output images.
    if (qAbs(sx-sy) * qMax(desktop.width(), desktop.height()) > 2) return {};
    const auto rect = screens[target];
    const int left = qRound((rect.x()-desktop.x())*sx), top = qRound((rect.y()-desktop.y())*sy);
    const int right = qRound((rect.x()+rect.width()-desktop.x())*sx);
    const int bottom = qRound((rect.y()+rect.height()-desktop.y())*sy);
    const QRect crop(left, top, right-left, bottom-top);
    return crop.isValid() && QRect(QPoint(),pixels).contains(crop) ? crop : QRect();
}
}
