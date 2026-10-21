#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QScreen>
#include <QJsonDocument>
#include <QTextStream>
#include <QTimer>
#include <QProcess>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QEnterEvent>
#include <QWheelEvent>
#include <memory>
#include <QJsonObject>
#include <QDir>
#include <QPainter>
#include <QDBusConnection>
#include <QDBusMessage>
#include "Controller.h"
#include "KeyboardPack.h"
#include "KeyboardLayout.h"
#include "SymbolProvider.h"
#include "PointerCheck.h"
#ifdef OPTIKEY_LINUX
#include <LayerShellQt/Window>
#include "PlasmaInputMethod.h"
#endif
class MagnifierProvider:public QQuickImageProvider {
public:
 explicit MagnifierProvider(Controller *controller):QQuickImageProvider(QQuickImageProvider::Image),controller(controller){}
 QImage requestImage(const QString &,QSize *size,const QSize &)override{
  auto result=controller->magnifierImage();if(size)*size=result.size();return result;
 }
private:
 Controller *controller;
};
class InputObserver:public QObject {
public:
 bool pointerEvents=false;
 bool eventFilter(QObject *object,QEvent *event)override{
  if(event->type()==QEvent::KeyPress){auto key=static_cast<QKeyEvent*>(event);QTextStream(stdout)<<QJsonDocument(QJsonObject{{"text",key->text()},{"key",key->key()}}).toJson(QJsonDocument::Compact)<<Qt::endl;}
  auto window=qobject_cast<QWindow*>(object);
  if(pointerEvents&&window&&window->width()>0&&window->height()>0){
   QJsonObject data;
   if(event->type()==QEvent::MouseButtonPress||event->type()==QEvent::MouseButtonDblClick||event->type()==QEvent::MouseButtonRelease||event->type()==QEvent::MouseMove){
    auto mouse=static_cast<QMouseEvent*>(event);
    data={{"pointer",event->type()==QEvent::MouseMove?"move":event->type()==QEvent::MouseButtonRelease?"release":"press"},{"double",event->type()==QEvent::MouseButtonDblClick},{"time",qint64(mouse->timestamp())},{"button",int(mouse->button())},{"buttons",int(mouse->buttons())},{"x",mouse->position().x()/window->width()},{"y",mouse->position().y()/window->height()}};
   }else if(event->type()==QEvent::Wheel){
    auto wheel=static_cast<QWheelEvent*>(event);const auto delta=wheel->pixelDelta().isNull()?wheel->angleDelta():wheel->pixelDelta();
    data={{"pointer","wheel"},{"dx",delta.x()},{"dy",delta.y()}};
   }
   if(!data.isEmpty())QTextStream(stdout)<<QJsonDocument(data).toJson(QJsonDocument::Compact)<<Qt::endl;
  }
  return false;
 }
};
int main(int argc,char **argv){
 bool inputMethodMode=false;for(int i=1;i<argc;++i)inputMethodMode|=QString::fromLocal8Bit(argv[i])=="--input-method";
 if(inputMethodMode&&qEnvironmentVariableIsEmpty("WAYLAND_SOCKET")){QTextStream(stderr)<<"Input-method mode must be launched by KWin through Virtual Keyboard settings or a nested test session.\n";return 2;}
 bool diagnose=false,audit=false,smoke=false,portalCheck=false,inputCheck=false,inputTarget=false,pointerCheck=false,pointerTarget=false;
 for(int i=1;i<argc;++i){QString arg=argv[i];diagnose|=arg=="--diagnose";audit|=arg=="--validate-keyboards";smoke|=arg=="--smoke";portalCheck|=arg=="--portal-check";inputCheck|=arg=="--input-check";inputTarget|=arg=="--input-target";pointerCheck|=arg=="--pointer-check";pointerTarget|=arg=="--pointer-target";}
 if(diagnose||audit)qputenv("QT_QPA_PLATFORM","offscreen");
 QApplication app(argc,argv);app.setOrganizationName("OptiKey");app.setApplicationName("OptiKey-Pro");app.setDesktopFileName("org.optikey.Pro");
 const bool magnifierCheck=app.arguments().contains("--magnifier-check");pointerCheck|=magnifierCheck;
 if(app.arguments().contains("--input-method-check")&&qgetenv("OPTIKEY_ISOLATED_IM_TEST")!="1"){
  QTextStream(stderr)<<"Run tests/native_input_method.py to isolate the native input check\n";return 2;
 }
 if(app.arguments().contains("--input-method-target")){
  QTextStream(stderr)<<"Native receiver platform: "<<QGuiApplication::platformName()<<Qt::endl;
  QQmlApplicationEngine targetEngine;targetEngine.load(QUrl("qrc:/ui/InputMethodTarget.qml"));if(targetEngine.rootObjects().isEmpty())return 1;
  auto target=targetEngine.rootObjects().front();QTimer poll;
  QObject::connect(&poll,&QTimer::timeout,&app,[&]{
   if(target->property("focusLost").toBool()){QTextStream(stderr)<<"Native receiver lost focus\n";app.exit(2);return;}
   if(target->property("privateComplete").toBool()&&target->property("receivedText").toString()==QString::fromUtf8("OptiKey é€😀\n")){
   QTextStream(stdout)<<"PASS: native Unicode commit, Backspace, Return and private field reached the Qt receiver"<<Qt::endl;app.quit();
  }});poll.start(100);QTimer::singleShot(20000,&app,[&]{QTextStream(stderr)<<"Native input-method receiver timed out\n";app.exit(2);});return app.exec();
 }
 if(inputTarget||pointerTarget){
  QQmlApplicationEngine targetEngine;targetEngine.load(QUrl(pointerTarget?"qrc:/ui/PointerTarget.qml":"qrc:/ui/InputTarget.qml"));
  if(targetEngine.rootObjects().isEmpty())return 1;
  auto target=qobject_cast<QQuickWindow*>(targetEngine.rootObjects().front());InputObserver observer;
  observer.pointerEvents=pointerTarget;
  target->installEventFilter(&observer);
  auto reportTarget=[target]{QTextStream(stdout)<<QJsonDocument(QJsonObject{{"active",target->isActive()},{"width",target->width()},{"height",target->height()}}).toJson(QJsonDocument::Compact)<<Qt::endl;};
  QObject::connect(target,&QWindow::activeChanged,&app,reportTarget);
  QObject::connect(target,&QWindow::widthChanged,&app,reportTarget);QObject::connect(target,&QWindow::heightChanged,&app,reportTarget);
  if(pointerTarget)target->showFullScreen();else target->show();target->requestActivate();QTimer::singleShot(pointerTarget?120000:60000,&app,&QCoreApplication::quit);return app.exec();
 }
 if(diagnose||audit){
  QVariantMap report;
  if(diagnose){report["tobii"]=TobiiInput::probe();report["portal"]=PortalInput::probe();report["qtVersion"]=qVersion();}
  if(audit)report["keyboards"]=KeyboardPack::audit(QStringLiteral(DEFAULT_PACK_PATH));
  QTextStream(stdout)<<QJsonDocument::fromVariant(report).toJson();
  return audit&&!report["keyboards"].toMap()["errors"].toStringList().isEmpty()?1:0;
 }
 if(portalCheck){
  PortalInput portal;
  QObject::connect(&portal,&PortalInput::status,&app,[](QString message){QTextStream(stdout)<<message<<Qt::endl;});
  QObject::connect(&portal,&PortalInput::readyChanged,&app,[&]{if(portal.ready())QTimer::singleShot(200,&app,&QCoreApplication::quit);});
  QTimer::singleShot(0,&portal,&PortalInput::connectPortal);
  QTimer::singleShot(60000,&app,[&]{QTextStream(stdout)<<"Portal check timed out"<<Qt::endl;app.exit(2);});
  return app.exec();
 }
 std::unique_ptr<PlasmaInputMethod> inputMethod;
 if(inputMethodMode){
  inputMethod=std::make_unique<PlasmaInputMethod>();
  if(!inputMethod->initialize()){QTextStream(stderr)<<"KWin input-method interfaces unavailable\n";return 2;}
  if(app.arguments().contains("--input-method-check"))QTextStream(stderr)<<"Native input-method interfaces bound"<<Qt::endl;
 }
 // Register before any portal request so remembered screenshot approval belongs
 // to OptiKey rather than an unidentified host application. Older portals may
 // not implement Registry; their normal permission dialogs remain available.
 bool registerIdentity=!smoke;
 for(const auto &arg:app.arguments())if(arg.endsWith("-check"))registerIdentity=false;
 if(registerIdentity){
  auto registration=QDBusMessage::createMethodCall("org.freedesktop.portal.Desktop","/org/freedesktop/portal/desktop","org.freedesktop.host.portal.Registry","Register");
  registration.setArguments({QString("org.optikey.Pro"),QVariantMap{}});
  QDBusConnection::sessionBus().call(registration,QDBus::Block,2000);
 }
 Controller controller;
 if(inputMethod){
  controller.configureInputMethod([&](QString text){return inputMethod->commitText(text);},[&](QString name,bool down){return inputMethod->sendKey(name,down);});
  QObject::connect(inputMethod.get(),&PlasmaInputMethod::contextInvalidated,&controller,&Controller::invalidateInputContext);
  QObject::connect(inputMethod.get(),&PlasmaInputMethod::stateChanged,&controller,[&]{controller.inputContextChanged(inputMethod->ready(),inputMethod->sensitive());});
 }
 int inputMethodCheckStage=0;
 QQuickWindow *nativeCheckWindow=nullptr;
 if(inputMethod&&app.arguments().contains("--input-method-check")){
  QObject::connect(inputMethod.get(),&PlasmaInputMethod::stateChanged,&controller,[&]{
   QTextStream(stderr)<<"Native test context: ready="<<inputMethod->ready()<<" private="<<inputMethod->sensitive()<<Qt::endl;
   if(!inputMethod->ready())return;
   const bool sensitive=inputMethod->sensitive();
   if((inputMethodCheckStage==0&&!sensitive)||(inputMethodCheckStage==1&&sensitive)){
    ++inputMethodCheckStage;
    QTimer::singleShot(100,&controller,[&,sensitive]{
     if(!nativeCheckWindow||!inputMethod->ready()||inputMethod->sensitive()!=sensitive){app.exit(2);return;}
     controller.openLayout(sensitive?"main-14x1-top":"main-14x1-bottom",false);
     auto timer=new QTimer(&controller);
     timer->setInterval(100);
     // Odd count ends on the opposite edge. Each phase exercises five
     // round trips plus one move, through the rendered QML button.
     QObject::connect(timer,&QTimer::timeout,&controller,[&,sensitive,timer,clicks=0,expected=QString()]()mutable{
      if(!inputMethod->ready()||inputMethod->sensitive()!=sensitive){timer->stop();timer->deleteLater();app.exit(2);return;}
      if(!expected.isEmpty()&&controller.property("layoutName").toString()!=expected){
       QTextStream(stderr)<<"Placement button did not navigate to "<<expected<<Qt::endl;
       timer->stop();timer->deleteLater();app.exit(2);return;
      }
      if(clicks==11){
       timer->stop();timer->deleteLater();
       controller.openLayout(sensitive?"keyboard-alphabet-14x4-bottom":"keyboard-alphabet-14x4-top",false);
       QTextStream(stderr)<<"Native placement check: "<<(sensitive?"bottom":"top")<<" (11 UI clicks)"<<Qt::endl;
       QVariantList commands;
       if(sensitive)commands={QVariantMap{{"tag","Text"},{"text","secret42"}}};
       else commands={QVariantMap{{"tag","Text"},{"text",QString::fromUtf8("OptiKey é€😀X")}},QVariantMap{{"tag","Action"},{"text","Backspace"}},QVariantMap{{"tag","Text"},{"text","\n"}}};
       if(!controller.executeCommands(commands)){QTextStream(stderr)<<"Native check could not start\n";app.exit(2);}
       return;
      }
      const auto layout=controller.property("layout").toMap();
      const auto key=layout["keys"].toList().front().toMap();
      QPointF point((key["col"].toDouble()+key["width"].toDouble()/2)*nativeCheckWindow->width()/layout["cols"].toInt(),
                    (key["row"].toDouble()+key["height"].toDouble()/2)*nativeCheckWindow->height()/layout["rows"].toInt());
      expected=layout["position"]=="Top"?"main-14x1-bottom":"main-14x1-top";
      QMouseEvent press(QEvent::MouseButtonPress,point,point,Qt::LeftButton,Qt::LeftButton,Qt::NoModifier);
      QMouseEvent release(QEvent::MouseButtonRelease,point,point,Qt::LeftButton,Qt::NoButton,Qt::NoModifier);
      QCoreApplication::sendEvent(nativeCheckWindow,&press);
      QCoreApplication::sendEvent(nativeCheckWindow,&release);
      ++clicks;
     });
     timer->start();
    });
   }
  });
 }
 if(inputCheck||pointerCheck){controller.setKeyboardScale(100);controller.openLayout("keyboard-alphabet-14x4-bottom",false);}
 auto args=app.arguments();int layoutIndex=args.indexOf("--layout");if(layoutIndex>=0&&layoutIndex+1<args.size())controller.openLayout(args[layoutIndex+1],false);
 QQmlApplicationEngine engine;
 engine.addImageProvider("symbols",new SymbolProvider);
 engine.addImageProvider("magnifier",new MagnifierProvider(&controller));
 engine.rootContext()->setContextProperty("controller",&controller);
 engine.load(QUrl("qrc:/ui/Main.qml"));if(engine.rootObjects().isEmpty())return 1;
 auto window=qobject_cast<QQuickWindow*>(engine.rootObjects().front());if(!window)return 1;
 nativeCheckWindow=window;
 if(app.arguments().contains("--gaze-indicator-ui-check")){
  auto overlay=window->findChild<QQuickWindow*>("gazeOverlay");
  if(!overlay)return 1;
  window->setProperty("gazeEnabled",true);controller.setProperty("gazeIndicatorMorph",true);overlay->setVisible(true);
  auto sample=[overlay](double x,double y){return QMetaObject::invokeMethod(overlay,"updateMotion",Q_ARG(QVariant,x),Q_ARG(QVariant,y),Q_ARG(QVariant,true));};
  if(!sample(.1,.1))return 1;
  QTimer::singleShot(30,&app,[&,overlay,sample]{
   sample(.11,.1);
   QTimer::singleShot(30,&app,[&,overlay,sample]{
    sample(.2,.1);
    const bool stretched=overlay->property("stretch").toDouble()>0&&overlay->property("stretch").toDouble()<=1;
    const bool horizontal=qAbs(overlay->property("heading").toDouble())<.001;
    fprintf(stderr,"indicator stretch=%f heading=%f visible=%d\n",overlay->property("stretch").toDouble(),overlay->property("heading").toDouble(),overlay->isVisible());
    QMetaObject::invokeMethod(overlay,"updateMotion",Q_ARG(QVariant,0.0),Q_ARG(QVariant,0.0),Q_ARG(QVariant,false));
    app.exit(stretched&&horizontal&&overlay->property("stretch").toDouble()==0?0:1);
   });
  });
  return app.exec();
 }
 auto settings=window->findChild<QQuickWindow*>("settingsWindow");
 if(!settings)return 1;
 settings->setTransientParent(nullptr);
 app.setQuitOnLastWindowClosed(false);
 QSystemTrayIcon tray(QIcon::fromTheme("input-keyboard",QIcon(":/resources/optikey-tray.svg")));
 tray.setToolTip("OptiKey Pro");
 QMenu trayMenu;
 auto openSettings=trayMenu.addAction(QIcon::fromTheme("configure"),"Settings…");
 QObject::connect(openSettings,&QAction::triggered,&controller,[&]{controller.activate("Settings");});
 trayMenu.addSeparator();
 auto quitAction=trayMenu.addAction(QIcon::fromTheme("application-exit"),"Quit OptiKey");
 QObject::connect(quitAction,&QAction::triggered,&app,&QApplication::quit);
 tray.setContextMenu(&trayMenu);
 QObject::connect(&tray,&QSystemTrayIcon::activated,&controller,[&](QSystemTrayIcon::ActivationReason reason){
  if(reason==QSystemTrayIcon::Trigger||reason==QSystemTrayIcon::DoubleClick)controller.activate("Settings");
 });
 tray.show();
 auto target=window->findChild<QQuickWindow*>("targetWindow");
 auto gazeOverlay=window->findChild<QQuickWindow*>("gazeOverlay");

 auto applyGeometry=[&]{
  auto screen=QGuiApplication::primaryScreen();
  if(!screen){controller.displayChanged();return;}
  auto size=screen->geometry().size();auto layout=controller.property("layout").toMap();
  controller.setDisplaySize(size.width(),size.height());
  auto dimension=[](QString value,double total){return value.endsWith('%')?value.chopped(1).toDouble()*total/100:value.toDouble();};
  int width=qMax(1,int(dimension(layout.value("width","100%").toString(),size.width())));
  int height=qMax(1,int(dimension(layout.value("height","46%").toString(),size.height())));
  if(controller.keyboardScale()!=100&&!controller.property("layoutName").toString().startsWith("minimized-off-screen-"))
   height=qBound(1,int(height*controller.keyboardScale()/100.0),qMax(1,size.height()-84));
  bool top=layout["position"].toString()=="Top";
  int offset=int(dimension(layout.value("verticaloffset","0").toString(),size.height()));
  const bool minimized=controller.property("layoutName").toString().startsWith("minimized-off-screen-");
  int x=(size.width()-width)/2+int(dimension(layout.value("horizontaloffset","0").toString(),size.width()));
  int y=(top?0:size.height()-height)+offset;
  if(minimized){
   width=height=qMin(controller.restoreSize(),qMin(size.width(),size.height()));
   const int corner=controller.restoreCorner();
   x=corner%2?size.width()-width:0;
   y=corner>=2?size.height()-height:0;
  }
  window->setScreen(screen);window->resize(width,height);
  window->setProperty("screenWidth",size.width());window->setProperty("screenHeight",size.height());window->setProperty("logicalX",x);window->setProperty("logicalY",y);
  if(gazeOverlay){gazeOverlay->setScreen(screen);gazeOverlay->resize(size);}
#ifdef OPTIKEY_LINUX
  if(QGuiApplication::platformName().startsWith("wayland")){
   if(inputMethod&&!inputMethod->attachPanel(window,top)){QTextStream(stderr)<<"Unable to assign keyboard surface role\n";app.exit(2);return;}
   if(!inputMethod||top){auto layer=LayerShellQt::Window::get(window);layer->setScope("optikey-pro-keyboard");layer->setScreen(screen);layer->setLayer(LayerShellQt::Window::LayerTop);
   layer->setAnchors(top?LayerShellQt::Window::AnchorTop:LayerShellQt::Window::AnchorBottom);
   layer->setMargins(top?QMargins(0,offset,0,0):QMargins(0,0,0,-offset));layer->setDesiredSize(QSize(width,height));
   layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);layer->setActivateOnShow(false);layer->setExclusiveZone(inputMethod||minimized?-1:0);
   if(minimized){
    layer->setLayer(LayerShellQt::Window::LayerOverlay);
    layer->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)|LayerShellQt::Window::AnchorLeft);
    layer->setMargins(QMargins(x,y,0,0));
   }}
   if(target){target->setScreen(screen);target->resize(size);auto overlay=LayerShellQt::Window::get(target);overlay->setScope("optikey-pro-target");overlay->setScreen(screen);overlay->setLayer(LayerShellQt::Window::LayerOverlay);overlay->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)|LayerShellQt::Window::AnchorLeft);overlay->setDesiredSize(size);overlay->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);overlay->setExclusiveZone(-1);}
   if(gazeOverlay){auto overlay=LayerShellQt::Window::get(gazeOverlay);overlay->setScope("optikey-pro-gaze");overlay->setScreen(screen);overlay->setLayer(LayerShellQt::Window::LayerOverlay);overlay->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)|LayerShellQt::Window::AnchorLeft);overlay->setDesiredSize(size);overlay->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);overlay->setActivateOnShow(false);overlay->setExclusiveZone(-1);}
  }else
#endif
  {window->setPosition(screen->geometry().topLeft()+QPoint(x,y));if(gazeOverlay)gazeOverlay->setPosition(screen->geometry().topLeft());}
 };
 QObject::connect(&controller,&Controller::layoutChanged,&app,applyGeometry);
 QObject::connect(&controller,&Controller::geometryPreferencesChanged,&app,applyGeometry);
 auto displayChanged=[&]{controller.displayChanged();applyGeometry();};
 auto watchScreen=[&](QScreen *screen){
  QObject::connect(screen,&QScreen::geometryChanged,&app,displayChanged);
  QObject::connect(screen,&QScreen::logicalDotsPerInchChanged,&app,displayChanged);
  QObject::connect(screen,&QScreen::orientationChanged,&app,displayChanged);
 };
 for(auto screen:QGuiApplication::screens())watchScreen(screen);
 QObject::connect(&app,&QGuiApplication::screenAdded,&app,[&](QScreen *screen){watchScreen(screen);displayChanged();});
 QObject::connect(&app,&QGuiApplication::screenRemoved,&app,displayChanged);
 QObject::connect(&app,&QGuiApplication::primaryScreenChanged,&app,displayChanged);
 applyGeometry();window->show();
 if(app.arguments().contains("--target-dwell-ui-check")||app.arguments().contains("--pointer-dwell-ui-check")){
  // Synthetic gaze/pointer motion and screenshot, without mouse-button events.
  const bool gazeInput=app.arguments().contains("--target-dwell-ui-check");
  controller.inputContextChanged(true,false);
  controller.setProperty("targeting",true);controller.setProperty("magnifierEnabled",true);
  controller.setProperty("progressiveZoom",true);controller.setProperty("zoomDurationMs",500);
  controller.setProperty("targetDwellEnabled",true);controller.setProperty("targetStartDwellMs",250);controller.setProperty("targetConfirmDwellMs",300);
  window->setProperty("gazeEnabled",gazeInput);window->setProperty("gazeValid",gazeInput);window->setProperty("gaze",QPointF(.7,.7));
  auto poll=new QTimer(&app);poll->setInterval(25);
  QObject::connect(poll,&QTimer::timeout,&app,[&,gazeInput,stage=0]()mutable{
   if(!gazeInput&&target->isVisible()){
    const QPointF point(target->width()*.7,target->height()*.7);
    QEnterEvent enter(point,point,point);QCoreApplication::sendEvent(target,&enter);
    QMouseEvent move(QEvent::MouseMove,point,point,Qt::NoButton,Qt::NoButton,Qt::NoModifier);QCoreApplication::sendEvent(target,&move);
   }
   if(stage==0&&controller.property("capturePending").toBool()){
    QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);frame.fill(Qt::green);
    QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame));stage=1;
   }else if(stage==1&&!controller.property("zoomAnimating").toBool()){
    if(!controller.property("magnifierReady").toBool()){QTextStream(stderr)<<"FAIL: target selected during zoom"<<Qt::endl;app.exit(2);return;}
    stage=2;
   }else if(stage==2&&!controller.property("magnifierReady").toBool()){
    controller.clickCommand("CancelTarget");
    QTextStream(stdout)<<"PASS: "<<(gazeInput?"gaze":"pointer hover")<<" dwell started zoom and confirmed the target without mouse clicks"<<Qt::endl;app.quit();
   }
  });
  poll->start();QTimer::singleShot(6000,&app,[&]{QTextStream(stderr)<<"FAIL: dwell target selection timed out"<<Qt::endl;app.exit(2);});
 }
 if(app.arguments().contains("--progressive-zoom-ui-check")){
  // One synthetic screenshot; no screen sharing or desktop capture.
  controller.setProperty("targeting",true);controller.setProperty("capturePending",true);
  controller.setProperty("progressiveZoom",true);controller.setProperty("zoomDurationMs",500);controller.setProperty("magnifierZoom",4);
  QImage frame(QGuiApplication::primaryScreen()->size(),QImage::Format_RGB32);frame.fill(Qt::blue);
  {QPainter painter(&frame);painter.fillRect(QRect(0,0,frame.width()/4,frame.height()/4),Qt::red);}
  QMetaObject::invokeMethod(&controller,"receiveMagnifierFrame",Qt::DirectConnection,Q_ARG(QImage,frame));
  auto poll=new QTimer(&app);poll->setInterval(150);
  QObject::connect(poll,&QTimer::timeout,&app,[&]{
   if(controller.property("zoomAnimating").toBool())return;
   const auto image=target->grabWindow();
   if(image.isNull()||image.pixelColor(int(image.width()*.79),int(image.height()*.79))!=QColor(Qt::red)||controller.property("magnifierRevision").toInt()<2){
    QTextStream(stderr)<<"FAIL: progressive zoom did not render the enlarged source"<<Qt::endl;app.exit(2);return;
   }
   controller.clickCommand("CancelTarget");
   if(target->isVisible()){QTextStream(stderr)<<"FAIL: progressive zoom remained visible after cancel"<<Qt::endl;app.exit(2);return;}
   QTextStream(stdout)<<"PASS: progressive zoom rendered the enlarged source and closed on cancel"<<Qt::endl;app.quit();
  });
  poll->start();
 }
 if(app.arguments().contains("--dwell-startup-check")){
  // Synthetic gaze over a navigation key: no tracker, portal or desktop output.
  controller.setProperty("paused",false);controller.setProperty("dwellMs",250);
  controller.openLayout("main-14x1-bottom",false);
  window->setProperty("gazeEnabled",true);
  openSettings->trigger();
  if(!settings->isVisible()||controller.property("paused").toBool()||app.quitOnLastWindowClosed()){
   QTextStream(stderr)<<"FAIL: tray Settings action did not open settings with input active and background lifetime"<<Qt::endl;return 2;
  }
  if(window->findChild<QQuickWindow*>("controlWindow")){QTextStream(stderr)<<"FAIL: separate controls window still exists"<<Qt::endl;return 2;}
  auto poll=new QTimer(&app);poll->setInterval(25);
  QObject::connect(poll,&QTimer::timeout,&app,[&,started=false]()mutable{
   if(!settings->isActive()){
    if(started){QTextStream(stderr)<<"FAIL: settings lost focus during startup dwell check"<<Qt::endl;app.exit(2);}
    return;
   }
   started=true;
   if(controller.property("layoutName").toString()=="main-14x1-top"){
    QTextStream(stdout)<<"PASS: gaze dwell activated navigation with settings focused at startup"<<Qt::endl;app.quit();return;
   }
   const auto layout=controller.property("layout").toMap();
   const auto key=layout["keys"].toList().front().toMap();
   const double x=window->property("logicalX").toDouble()+(key["col"].toDouble()+key["width"].toDouble()/2)*window->width()/layout["cols"].toInt();
   const double y=window->property("logicalY").toDouble()+(key["row"].toDouble()+key["height"].toDouble()/2)*window->height()/layout["rows"].toInt();
   emit controller.gazePoint(x/window->property("screenWidth").toDouble(),y/window->property("screenHeight").toDouble(),true);
  });
  poll->start();QTimer::singleShot(4000,&app,[&]{QTextStream(stderr)<<"FAIL: startup dwell timed out"<<Qt::endl;app.exit(2);});
 }
 if(app.arguments().contains("--dwell-minimize-check")){
  controller.setRestoreCorner(0);controller.setRestoreSize(40);
  controller.setProperty("paused",false);controller.setProperty("dwellMs",250);
  controller.openLayout("main-14x1-bottom",false);window->setProperty("gazeEnabled",true);
  auto poll=new QTimer(&app);poll->setInterval(25);
  QObject::connect(poll,&QTimer::timeout,&app,[&,stage=0,corner=0]()mutable{
   const QStringList expected{"minimized-off-screen-bottom","main-14x1-bottom","minimized-off-screen-top","main-14x1-top"};
   if(controller.property("layoutName").toString().startsWith("minimized-off-screen-")){
    const int expectedX=corner%2?window->property("screenWidth").toInt()-window->width():0;
    const int expectedY=corner>=2?window->property("screenHeight").toInt()-window->height():0;
    if(window->property("logicalX").toInt()!=expectedX||window->property("logicalY").toInt()!=expectedY){
     QTextStream(stderr)<<"FAIL: restore square is not flush with its screen corner"<<Qt::endl;app.exit(2);return;
    }
   }
   if(controller.property("layoutName").toString()==expected[stage]){
    if(++stage==4){
     if(++corner==4){QTextStream(stdout)<<"PASS: gaze minimizes and restores both keyboard edges in all four corners"<<Qt::endl;app.quit();return;}
     stage=0;controller.setRestoreCorner(corner);controller.setRestoreSize(corner%2?128:40);controller.openLayout("main-14x1-bottom",false);
    }
    if(stage==2)controller.openLayout("main-14x1-top",false);
   }
   const auto layout=controller.property("layout").toMap();
   const auto keys=layout["keys"].toList();
   const auto key=keys[stage%2?0:keys.size()-1].toMap();
   const double x=window->property("logicalX").toDouble()+(key["col"].toDouble()+key["width"].toDouble()/2)*window->width()/layout["cols"].toInt();
   const double y=window->property("logicalY").toDouble()+(key["row"].toDouble()+key["height"].toDouble()/2)*window->height()/layout["rows"].toInt();
   const double nx=x/window->property("screenWidth").toDouble(),ny=y/window->property("screenHeight").toDouble();
   if(nx<0||nx>1||ny<0||ny>1){QTextStream(stderr)<<"FAIL: recovery key is outside the display"<<Qt::endl;app.exit(2);return;}
   emit controller.gazePoint(nx,ny,true);
  });
  poll->start();QTimer::singleShot(15000,&app,[&]{QTextStream(stderr)<<"FAIL: minimize/restore dwell timed out"<<Qt::endl;app.exit(2);});
 }
 if(app.arguments().contains("--gaze")){window->setProperty("gazeEnabled",true);controller.setGazeEnabled(true);}
 if(app.arguments().contains("--gaze-check")){
  // Exercise the real worker, controller freshness filter and QML signal handler,
  // without authorizing desktop output or allowing dwell actions.
  controller.setProperty("paused",true);
  window->setProperty("gazeCheck",true);
  window->setProperty("gazeEnabled",true);
  auto counts=std::make_shared<QJsonObject>(QJsonObject{{"signals",0},{"controllerValid",0},{"qmlValid",0},{"overKeyboard",0}});
  QObject::connect(&controller,&Controller::gazePoint,&app,[&,counts](double x,double y,bool valid){
   auto increment=[&](const char *key){(*counts)[key]=(*counts)[key].toInt()+1;};
   increment("signals");if(!valid)return;increment("controllerValid");
   if(window->property("gazeValid").toBool())increment("qmlValid");
   double px=x*window->property("screenWidth").toDouble()-window->property("logicalX").toDouble();
   double py=y*window->property("screenHeight").toDouble()-window->property("logicalY").toDouble();
   if(px>=0&&py>=0&&px<window->width()&&py<window->height())increment("overKeyboard");
  });
  controller.setGazeEnabled(true);
  QTimer::singleShot(20000,&app,[&,counts]{
   (*counts)["tobiiStatus"]=controller.property("tobiiStatus").toString();
   (*counts)["screenWidth"]=window->property("screenWidth").toInt();
   (*counts)["screenHeight"]=window->property("screenHeight").toInt();
   QTextStream(stdout)<<QJsonDocument(*counts).toJson()<<Qt::endl;
   app.exit((*counts)["qmlValid"].toInt()>0?0:2);
  });
 }
 QProcess receiver;QByteArray receiverBuffer;QString received;bool sent=false,active=false,clickStage=false;
 std::unique_ptr<PointerCheck> pointerHarness;
 if(pointerCheck){pointerHarness=std::make_unique<PointerCheck>(&controller,magnifierCheck);pointerHarness->start();}
 if(inputCheck){
  QObject::connect(controller.output(),&PortalInput::status,&app,[](QString message){QTextStream(stdout)<<message<<Qt::endl;});
  QObject::connect(controller.output(),&PortalInput::readyChanged,&app,[&]{
   if(controller.ready()&&receiver.state()==QProcess::NotRunning){receiver.setProcessChannelMode(QProcess::SeparateChannels);receiver.start(QCoreApplication::applicationFilePath(),{"--input-target"});}
  });
  QObject::connect(&receiver,&QProcess::readyReadStandardOutput,&app,[&]{
   receiverBuffer+=receiver.readAllStandardOutput();
   while(receiverBuffer.contains('\n')){
    auto end=receiverBuffer.indexOf('\n');auto line=receiverBuffer.left(end);receiverBuffer.remove(0,end+1);
    auto obj=QJsonDocument::fromJson(line).object();
    if(obj.contains("active")){
     active=obj["active"].toBool();
     if(!active&&sent){
      if(!controller.property("paused").toBool())controller.togglePause();
      QTextStream(stdout)<<"Input check stopped: disposable receiver lost focus"<<Qt::endl;app.exit(7);return;
     }
     if(active&&!sent)QTimer::singleShot(300,&app,[&]{
      if(!active||sent||!controller.ready())return;sent=true;
      // Send only after the disposable receiver reports keyboard focus.
      auto layout=KeyboardLayout::fromXml(KeyboardPack::load(QStringLiteral(DEFAULT_PACK_PATH)+"/keyboard-alphabet-14x4-bottom.xml"));
      QVariantList sequence;
      for(auto letter:QString("optikey")){
       bool found=false;
       for(auto v:layout["keys"].toList()){auto key=v.toMap();if(key["label"].toString()==QString(letter)){sequence+=key["commands"].toList();found=true;break;}}
       if(!found){app.exit(4);return;}
      }
      sequence<<QVariantMap{{"tag","Wait"},{"text","50"}}<<QVariantMap{{"tag","Text"},{"text","\r"}};
      if(!controller.executeCommands(sequence))app.exit(4);
     });
    }
    if(obj.contains("text")){
     received+=obj["text"].toString();
     if(obj["key"].toInt()==Qt::Key_Return){
      bool passed=received=="optikey\r";
      QTextStream(stdout)<<(passed?"PASS: imported XML actions delivered optikey + Enter to separate Wayland receiver":"FAIL: unexpected input sequence")<<Qt::endl;
      if(!passed){app.exit(5);return;}
      if(!controller.output()->absoluteAvailable()){QTextStream(stdout)<<"Click/focus check unavailable: no unique absolute EIS region"<<Qt::endl;app.exit(6);return;}
      clickStage=true;
      QTimer::singleShot(100,&app,[&]{
       if(!active||!controller.ready()){app.exit(7);return;}
       auto layout=controller.property("layout").toMap();
       for(auto value:layout["keys"].toList()){
        auto key=value.toMap();if(key["label"]!="q")continue;
        double px=window->property("logicalX").toDouble()+(key["col"].toDouble()+key["width"].toDouble()/2)*window->width()/layout["cols"].toInt();
        double py=window->property("logicalY").toDouble()+(key["row"].toDouble()+key["height"].toDouble()/2)*window->height()/layout["rows"].toInt();
        if(!controller.output()->moveAbsolute(px/window->property("screenWidth").toDouble(),py/window->property("screenHeight").toDouble())){app.exit(6);return;}
        QTimer::singleShot(100,&app,[&]{if(!active||!controller.ready()){app.exit(7);return;}if(!controller.output()->click())app.exit(6);});return;
       }
       app.exit(6);
      });
     }else if(clickStage&&obj["text"].toString()=="q"){
      QTextStream(stdout)<<"PASS: clicking the XML q key preserved receiver focus and delivered q"<<Qt::endl;app.exit(0);
     }
    }
   }
  });
  QTimer::singleShot(0,controller.output(),&PortalInput::connectPortal);
  QTimer::singleShot(60000,&app,[&]{QTextStream(stdout)<<"Input check timed out; focus the disposable receiver if it appeared"<<Qt::endl;app.exit(2);});
 }
 if(args.contains("--settings"))controller.activate("Settings");
 // Diagnostic modes manage their own sessions or must remain input-free.
 bool diagnosticMode=smoke;
 for(const auto &arg:args)if(arg.endsWith("-check"))diagnosticMode=true;
 if(!diagnosticMode&&controller.property("rememberDesktop").toBool())
  QTimer::singleShot(0,&controller,&Controller::connectDesktop);
 if(smoke)QTimer::singleShot(1500,&app,[&]{
  auto output=qEnvironmentVariable("OPTIKEY_SCREENSHOT");
  auto captureWindow=args.contains("--settings")?window->findChild<QQuickWindow*>("settingsWindow"):window;
  if(!output.isEmpty()&&(!captureWindow||!captureWindow->grabWindow().save(output))){QTextStream(stderr)<<"Screenshot failed"<<Qt::endl;app.exit(3);return;}
  app.quit();
 });
 int result=app.exec();
 if(receiver.state()!=QProcess::NotRunning){receiver.terminate();if(!receiver.waitForFinished(1500)){receiver.kill();receiver.waitForFinished();}}
 return result;
}
