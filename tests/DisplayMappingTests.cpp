#include <QtTest>
#include <QImage>
#include "DisplayMapping.h"
class DisplayMappingTests:public QObject {
 Q_OBJECT
private slots:
 void singleDisplayPreservesNormalizedCoordinates(){
  auto m=DisplayMapping::input({{0,0,1920,1080}},0,{{100,200,3840,2160}});
  QVERIFY(m.valid());QCOMPARE(m.point({0,0}),QPointF(100,200));QCOMPARE(m.point({1,1}),QPointF(3939,2359));
 }
 void selectsPrimaryRegionIndependentOfDeviceOrder(){
  const QList<QRectF> screens{{0,0,1920,1080},{1920,0,1920,1080}};
  auto m=DisplayMapping::input(screens,1,{{1920,0,1920,1080},{0,0,1920,1080}});
  QVERIFY(m.valid());QCOMPARE(m.region,0);QCOMPARE(m.point({0,0}),QPointF(1920,0));
  QCOMPARE(m.point({1,1}),QPointF(3839,1079));
 }
 void handlesNegativeOriginsAndGaps(){
  auto m=DisplayMapping::input({{-1800,-1000,1600,900},{0,0,1920,1080}},1,
                              {{1800,1000,1920,1080},{0,0,1600,900}});
  QVERIFY(m.valid());QCOMPARE(m.region,0);QCOMPARE(m.target,QRectF(1800,1000,1920,1080));
 }
 void handlesMixedMonitorScalesInLogicalCoordinates(){
  // A 2560x1440 output at 200% has a 1280x720 logical region.
  auto m=DisplayMapping::input({{0,0,1920,1080},{1920,0,1280,720}},1,
                              {{1920,0,1280,720},{0,0,1920,1080}});
  QVERIFY(m.valid());QCOMPARE(m.target,QRectF(1920,0,1280,720));
  QCOMPARE(m.point({.5,.5}),QPointF(2559.5,359.5));
 }
 void handlesUniformlyScaledInputCoordinateSpace(){
  auto m=DisplayMapping::input({{0,0,1920,1080},{1920,0,1280,720}},1,
                              {{0,0,3840,2160},{3840,0,2560,1440}});
  QVERIFY(m.valid());QCOMPARE(m.region,1);QCOMPARE(m.point({0,0}),QPointF(3840,0));
 }
 void handlesOneRegionCoveringWorkspace(){
  auto m=DisplayMapping::input({{-1920,0,1920,1080},{0,0,1920,1080}},1,{{0,0,3840,1080}});
  QVERIFY(m.valid());QCOMPARE(m.target,QRectF(1920,0,1920,1080));
 }
 void refusesAmbiguousAndMismatchedRegions(){
  const QList<QRectF> screens{{0,0,1920,1080},{1920,0,1920,1080}};
  QVERIFY(!DisplayMapping::input(screens,0,{{0,0,1920,1080},{0,0,1920,1080}}).valid());
  QVERIFY(!DisplayMapping::input(screens,0,{{0,0,1920,1080},{2000,0,1920,1080}}).valid());
  QVERIFY(!DisplayMapping::input(screens,0,{}).valid());
  QVERIFY(!DisplayMapping::input(screens,-1,{{0,0,3840,1080}}).valid());
 }
 void rejectsInvalidNormalizedPoints(){
  auto m=DisplayMapping::input({{0,0,100,100}},0,{{0,0,100,100}});
  QCOMPARE(m.point({-0.1,.5}),QPointF(-1,-1));
  QCOMPARE(m.point({qQNaN(),.5}),QPointF(-1,-1));
 }
 void cropsOffsetPrimaryFromWorkspaceImage(){
  const QList<QRectF> screens{{-100,0,100,80},{0,0,120,80}};
  QImage image(440,160,QImage::Format_RGB32);image.fill(Qt::red);
  for(int y=0;y<160;++y)for(int x=200;x<440;++x)image.setPixelColor(x,y,Qt::blue);
  const auto crop=DisplayMapping::screenshot(screens,1,image.size());
  QCOMPARE(crop,QRect(200,0,240,160));
  auto primary=image.copy(crop);QCOMPARE(primary.pixelColor(0,0),QColor(Qt::blue));
  QCOMPARE(primary.pixelColor(primary.width()-1,primary.height()-1),QColor(Qt::blue));
 }
 void cropsFractionalScaleMixedDpiWorkspace(){
  const QList<QRectF> screens{{0,0,1920,1080},{1920,0,1280,720}};
  QCOMPARE(DisplayMapping::screenshot(screens,1,{4800,1620}),QRect(2880,0,1920,1080));
 }
 void cropsVerticalArrangementAndEmptyGaps(){
  const QList<QRectF> screens{{-200,-900,1600,900},{0,0,1920,1080}};
  QCOMPARE(DisplayMapping::screenshot(screens,1,{3180,2970}),QRect(300,1350,2880,1620));
 }
 void rejectsImagesThatDoNotMatchTheWorkspace(){
  const QList<QRectF> screens{{0,0,1920,1080},{1920,0,1920,1080}};
  QVERIFY(!DisplayMapping::screenshot(screens,0,{1920,1080}).isValid());
  QVERIFY(!DisplayMapping::screenshot(screens,0,{}).isValid());
  QVERIFY(!DisplayMapping::screenshot({},0,{1920,1080}).isValid());
 }
};
QTEST_GUILESS_MAIN(DisplayMappingTests)
#include "DisplayMappingTests.moc"
