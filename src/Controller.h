#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QVariantList>
#include <QJsonObject>
#include "PortalInput.h"
#include "TobiiInput.h"
#include "Dwell.h"
#include "ActionRunner.h"
#include "PointerActions.h"
#include "ScreenshotPortal.h"
#include "SpeechOutput.h"
class Controller:public QObject{
 Q_OBJECT
 Q_PROPERTY(QString portalStatus MEMBER portalStatus NOTIFY changed)
 Q_PROPERTY(QString tobiiStatus MEMBER tobiiStatus NOTIFY changed)
 Q_PROPERTY(QString actionStatus MEMBER actionStatus NOTIFY changed)
 Q_PROPERTY(QString profileStatus MEMBER profileStatus NOTIFY changed)
 Q_PROPERTY(bool ready READ ready NOTIFY changed)
 Q_PROPERTY(bool paused MEMBER paused NOTIFY changed)
 Q_PROPERTY(int dwellMs MEMBER dwellMs NOTIFY changed)
 Q_PROPERTY(int keyboardScale READ keyboardScale WRITE setKeyboardScale NOTIFY geometryPreferencesChanged)
 Q_PROPERTY(bool highContrast MEMBER highContrast NOTIFY changed)
 Q_PROPERTY(bool rememberDesktop MEMBER rememberDesktop NOTIFY changed)
 Q_PROPERTY(QVariantMap layout MEMBER layout NOTIFY layoutChanged)
 Q_PROPERTY(QString layoutName MEMBER layoutName NOTIFY layoutChanged)
 Q_PROPERTY(QStringList layoutNames MEMBER layoutNames CONSTANT)
 Q_PROPERTY(QStringList menuLayoutNames MEMBER menuLayoutNames CONSTANT)
 Q_PROPERTY(bool shifted READ shifted NOTIFY changed)
 Q_PROPERTY(QStringList heldKeys READ heldKeys NOTIFY changed)
 Q_PROPERTY(bool literal MEMBER literal NOTIFY changed)
 Q_PROPERTY(QString mappingJson READ mappingJson NOTIFY changed)
 Q_PROPERTY(QString scratchpad MEMBER scratchpad NOTIFY changed)
 Q_PROPERTY(QStringList suggestions MEMBER suggestions NOTIFY changed)
 Q_PROPERTY(bool learning MEMBER learning NOTIFY changed)
 Q_PROPERTY(bool targeting MEMBER targeting NOTIFY changed)
 Q_PROPERTY(QString targetPrompt READ targetPrompt NOTIFY changed)
 Q_PROPERTY(QStringList heldPointerButtons READ heldPointerButtons NOTIFY changed)
 Q_PROPERTY(bool magneticCursor READ magneticCursor NOTIFY changed)
 Q_PROPERTY(bool lookScrolling READ lookScrolling NOTIFY changed)
 Q_PROPERTY(bool magnifierEnabled MEMBER magnifierEnabled NOTIFY changed)
 Q_PROPERTY(bool capturePending MEMBER capturePending NOTIFY changed)
 Q_PROPERTY(bool magnifierReady MEMBER magnifierReady NOTIFY changed)
 Q_PROPERTY(int magnifierRevision MEMBER magnifierRevision NOTIFY changed)
public:
 Controller();~Controller()override;
 PortalInput *output(){return &portal;}
 bool ready()const{return nativeActive||portal.ready();}
 void configureInputMethod(std::function<bool(QString)> text,std::function<bool(QString,bool)> key);
 void inputContextChanged(bool active,bool sensitive);
 void invalidateInputContext();
 int keyboardScale()const{return scalePercent;}
 void setKeyboardScale(int scale);
 bool shifted()const{return runner.shifted();}
 QStringList heldKeys()const{return runner.heldKeys();}
 QString mappingJson()const;
 QString targetPrompt()const{return magnifierReady?"Choose the precise target in the magnified image":pointer.prompt();}
 QImage magnifierImage()const{return magnifiedImage;}
 QStringList heldPointerButtons()const{return pointer.heldButtons();}
 bool magneticCursor()const{return pointer.magnetic();}
 bool lookScrolling()const{return pointer.lookScrolling();}
 Q_INVOKABLE void setGazeEnabled(bool enabled);
 Q_INVOKABLE void trackGaze(double x,double y,bool overDesktop);
 void displayChanged();
 void setDisplaySize(double width,double height){pointer.setDisplaySize({width,height});}
 Q_INVOKABLE bool saveMappings(QString json);
 Q_INVOKABLE bool savePreferences();
 Q_INVOKABLE void resetPreferences();
 Q_INVOKABLE void connectDesktop(){portal.setRememberPermission(rememberDesktop);portal.connectPortal();}
 Q_INVOKABLE void forgetDesktopPermission(){portal.forgetPermission();rememberDesktop=false;emit changed();}
 Q_INVOKABLE void connectTobii(){if(!tobii.isRunning())tobii.start();}
 Q_INVOKABLE void togglePause();
 Q_INVOKABLE void activate(QString key);
 Q_INVOKABLE void clickCommand(QString key);
 Q_INVOKABLE double hover(QString key);
 Q_INVOKABLE void openLayout(QString name,bool backReturnsHere=true);
 Q_INVOKABLE void recover();
 Q_INVOKABLE void targetAt(double x,double y);
 Q_INVOKABLE void resetLearning();
 Q_INVOKABLE void speak();
 Q_INVOKABLE void stopSpeech();
 // Used only by deterministic external-input integration checks.
 bool executeCommands(const QVariantList &commands){return runner.start(commands);}
 bool executingCommands()const{return runner.busy();}
signals:
 void geometryPreferencesChanged();
 void settingsRequested();
 void changed();
 void layoutChanged();
 void gazePoint(double x,double y,bool valid);
private:
 bool performAction(QString action);
 void clearCapture();
 void updateText(QString text);
 void updateSuggestions();
 QVariantList mapped(const QVariantList &commands,bool &ok);
 PortalInput portal;TobiiInput tobii;Dwell dwell;ActionRunner runner;PointerActions pointer;QElapsedTimer clock,gazeAge;
 ScreenshotPortal screenshot;
 SpeechOutput speech;
 QImage magnifiedImage;
 QRect magnifierCrop;
 QSize captureSize;
 QPointF coarseTarget;
 bool magnifierEnabled=false,capturePending=false,magnifierReady=false;
 int magnifierRevision=0;
 quint64 captureEpoch=0;
 QString portalStatus="Desktop control disconnected",tobiiStatus="Tobii ready to connect",actionStatus="Choose Authorize to enable desktop output";
 QString profileStatus;
 bool paused=false,literal=false,learning=false,targeting=false,gazeEnabled=false;
 int dwellMs=750;
 int scalePercent=100;
 bool highContrast=false;
 bool rememberDesktop=false;
 bool nativeMode=false,nativeActive=false,sensitiveInput=false;
 qint64 lastGazeSample=0;
 QVariantMap layout;QString layoutName;QStringList layoutNames,menuLayoutNames,backStack;
 QString scratchpad;QStringList suggestions,dictionary;QMap<QString,int> frequencies;
 QJsonObject mappingProfile;
};
