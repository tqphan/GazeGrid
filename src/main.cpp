#include <QGuiApplication>
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
#include <QWheelEvent>
#include <memory>
#include <QJsonObject>
#include <QDir>
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
 QGuiApplication app(argc,argv);app.setOrganizationName("OptiKey");app.setApplicationName("OptiKey-Pro");app.setDesktopFileName("org.optikey.Pro");
 const bool magnifierCheck=app.arguments().contains("--magnifier-check");pointerCheck|=magnifierCheck;
 if(app.arguments().contains("--input-method-check")&&qgetenv("OPTIKEY_ISOLATED_IM_TEST")!="1"){
  QTextStream(stderr)<<"Run tests/native_input_method.py to isolate the native input check\n";return 2;
 }
 if(app.arguments().contains("--input-method-target")){
  QTextStream(stderr)<<"Native receiver platform: "<<QGuiApplication::platformName()<<Qt::endl;
  QQmlApplicationEngine targetEngine;targetEngine.load(QUrl("qrc:/ui/InputMethodTarget.qml"));if(targetEngine.rootObjects().isEmpty())return 1;
  auto target=targetEngine.rootObjects().front();QTimer poll;
  QObject::connect(&poll,&QTimer::timeout,&app,[&]{if(target->property("privateComplete").toBool()&&target->property("receivedText").toString()==QString::fromUtf8("OptiKey é€😀\n")){
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
 Controller controller;
 if(inputMethod){
  controller.configureInputMethod([&](QString text){return inputMethod->commitText(text);},[&](QString name,bool down){return inputMethod->sendKey(name,down);});
  QObject::connect(inputMethod.get(),&PlasmaInputMethod::contextInvalidated,&controller,&Controller::invalidateInputContext);
  QObject::connect(inputMethod.get(),&PlasmaInputMethod::stateChanged,&controller,[&]{controller.inputContextChanged(inputMethod->ready(),inputMethod->sensitive());});
 }
 int inputMethodCheckStage=0;
 if(inputMethod&&app.arguments().contains("--input-method-check")){
  QObject::connect(inputMethod.get(),&PlasmaInputMethod::stateChanged,&controller,[&]{
   QTextStream(stderr)<<"Native test context: ready="<<inputMethod->ready()<<" private="<<inputMethod->sensitive()<<Qt::endl;
   if(!inputMethod->ready())return;
   const bool sensitive=inputMethod->sensitive();
   if((inputMethodCheckStage==0&&!sensitive)||(inputMethodCheckStage==1&&sensitive)){
    ++inputMethodCheckStage;
    QTimer::singleShot(100,&controller,[&,sensitive]{
     if(!inputMethod->ready()||inputMethod->sensitive()!=sensitive)return;
     QVariantList commands;
     if(sensitive)commands={QVariantMap{{"tag","Text"},{"text","secret42"}}};
     else commands={QVariantMap{{"tag","Text"},{"text",QString::fromUtf8("OptiKey é€😀X")}},QVariantMap{{"tag","Action"},{"text","Backspace"}},QVariantMap{{"tag","Text"},{"text","\n"}}};
     if(!controller.executeCommands(commands)){QTextStream(stderr)<<"Native check could not start\n";app.exit(2);}
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
 auto controls=window->findChild<QQuickWindow*>("controlWindow");
 auto target=window->findChild<QQuickWindow*>("targetWindow");
 if(inputMethod&&!inputMethod->attachPanel(window)){QTextStream(stderr)<<"Unable to assign input-panel role\n";return 2;}
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
  int x=(size.width()-width)/2+int(dimension(layout.value("horizontaloffset","0").toString(),size.width()));
  int y=(top?0:size.height()-height)+offset;
  window->setScreen(screen);window->resize(width,height);
  window->setProperty("screenWidth",size.width());window->setProperty("screenHeight",size.height());window->setProperty("logicalX",x);window->setProperty("logicalY",y);
#ifdef OPTIKEY_LINUX
  if(QGuiApplication::platformName().startsWith("wayland")){
   if(!inputMethod){auto layer=LayerShellQt::Window::get(window);layer->setScope("optikey-pro-keyboard");layer->setScreen(screen);layer->setLayer(LayerShellQt::Window::LayerTop);
   layer->setAnchors(top?LayerShellQt::Window::AnchorTop:LayerShellQt::Window::AnchorBottom);
   layer->setMargins(top?QMargins(0,offset,0,0):QMargins(0,0,0,-offset));layer->setDesiredSize(QSize(width,height));
   layer->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);layer->setActivateOnShow(false);layer->setExclusiveZone(0);}
   if(controls){controls->setScreen(screen);controls->resize(size.width(),84);auto bar=LayerShellQt::Window::get(controls);bar->setScope("optikey-pro-controls");bar->setScreen(screen);bar->setLayer(LayerShellQt::Window::LayerOverlay);bar->setAnchors(top?LayerShellQt::Window::AnchorBottom:LayerShellQt::Window::AnchorTop);bar->setDesiredSize(QSize(size.width(),84));bar->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);bar->setActivateOnShow(false);bar->setExclusiveZone(84);}
   if(target){target->setScreen(screen);target->resize(size);auto overlay=LayerShellQt::Window::get(target);overlay->setScope("optikey-pro-target");overlay->setScreen(screen);overlay->setLayer(LayerShellQt::Window::LayerOverlay);overlay->setAnchors(LayerShellQt::Window::Anchors(LayerShellQt::Window::AnchorTop)|LayerShellQt::Window::AnchorLeft);overlay->setDesiredSize(size);overlay->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);overlay->setExclusiveZone(-1);}
  }else
#endif
  {window->setPosition(screen->geometry().topLeft()+QPoint(x,y));if(controls)controls->setPosition(screen->geometry().topLeft()+QPoint(0,top?size.height()-84:0));}
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
 applyGeometry();window->show();if(controls)controls->show();
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
