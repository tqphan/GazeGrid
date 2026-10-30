// Regenerate the Windows ICO and PNG preview from the editable SVG.
// Build with Qt6 Core, Gui and Svg; run: generate_app_icon <resources-directory>
#include <QCoreApplication>
#include <QSvgRenderer>
#include <QImage>
#include <QPainter>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QTextStream>
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    if (argc != 2) return 1;
    QDir resources(QString::fromLocal8Bit(argv[1]));
    QSvgRenderer svg(resources.filePath("gaze-grid.svg"));
    if (!svg.isValid()) return 2;
    const QList<int> sizes{16, 20, 24, 32, 40, 48, 64, 128, 256};
    QList<QByteArray> images;
    for (int size : sizes) {
        QImage image(size, size, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::transparent);
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        svg.render(&painter);
        painter.end();
        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly) || !image.save(&buffer, "PNG")) return 3;
        images.append(png);
    }
    QFile icon(resources.filePath("gaze-grid.ico"));
    if (!icon.open(QIODevice::WriteOnly)) return 4;
    QDataStream output(&icon);
    output.setByteOrder(QDataStream::LittleEndian);
    output << quint16(0) << quint16(1) << quint16(sizes.size());
    quint32 offset = 6 + 16 * sizes.size();
    for (int i = 0; i < sizes.size(); ++i) {
        const quint8 dimension = sizes[i] == 256 ? 0 : sizes[i];
        output << dimension << dimension << quint8(0) << quint8(0)
               << quint16(1) << quint16(32) << quint32(images[i].size()) << offset;
        offset += images[i].size();
    }
    for (const auto &png : images) output.writeRawData(png.constData(), png.size());
    if (output.status() != QDataStream::Ok) return 5;
    icon.close();
    QImage preview(512, 512, QImage::Format_ARGB32_Premultiplied);
    preview.fill(Qt::transparent);
    QPainter painter(&preview);
    svg.render(&painter);
    painter.end();
    if (!preview.save(resources.filePath("gaze-grid.png"))) return 6;
    QTextStream(stdout) << "Generated PNG and nine-size ICO from gaze-grid.svg\n";
}
