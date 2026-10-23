#pragma once
#include <QObject>
#include <QElapsedTimer>
#include <QVariantList>
#include <QJsonObject>
#include "PortalInput.h"
#include "TobiiInput.h"
#include "Dwell.h"
#include "PointDwell.h"
#include "GazeSmoothing.h"
#include "ActionRunner.h"
#include "PointerActions.h"
#include "GameController.h"
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
 Q_PROPERTY(bool gazeEnabled READ isGazeEnabled WRITE setGazeEnabled NOTIFY gazeEnabledChanged)
 Q_PROPERTY(int gazeSmoothingLevel READ gazeSmoothingLevel WRITE setGazeSmoothingLevel NOTIFY changed)
 Q_PROPERTY(bool smoothWhenChangingGazeTarget READ smoothWhenChangingGazeTarget WRITE setSmoothWhenChangingGazeTarget NOTIFY changed)
 Q_PROPERTY(int gazeIndicatorSize MEMBER gazeIndicatorSize NOTIFY changed)
 Q_PROPERTY(int gazeIndicatorOpacity MEMBER gazeIndicatorOpacity NOTIFY changed)
 Q_PROPERTY(QString gazeIndicatorColor MEMBER gazeIndicatorColor NOTIFY changed)
 Q_PROPERTY(bool gazeIndicatorMorph MEMBER gazeIndicatorMorph NOTIFY changed)
 Q_PROPERTY(int repeatIntervalMs MEMBER repeatIntervalMs NOTIFY changed)
 Q_PROPERTY(bool repeatPunctuationOnHover MEMBER repeatPunctuationOnHover NOTIFY changed)
 Q_PROPERTY(bool repeatLettersOnHover MEMBER repeatLettersOnHover NOTIFY changed)
 Q_PROPERTY(bool repeatBackspaceOnHover MEMBER repeatBackspaceOnHover NOTIFY changed)
 Q_PROPERTY(bool repeatArrowsOnHover MEMBER repeatArrowsOnHover NOTIFY changed)
 Q_PROPERTY(bool cycleModifiersOnHover MEMBER cycleModifiersOnHover NOTIFY changed)
 Q_PROPERTY(int dwellMs MEMBER dwellMs NOTIFY changed)
 Q_PROPERTY(int restoreCorner READ restoreCorner WRITE setRestoreCorner NOTIFY geometryPreferencesChanged)
 Q_PROPERTY(int restoreOpacity MEMBER restoreOpacity NOTIFY changed)
 Q_PROPERTY(int restoreWidth READ restoreWidth WRITE setRestoreWidth NOTIFY geometryPreferencesChanged)
 Q_PROPERTY(int restoreHeight READ restoreHeight WRITE setRestoreHeight NOTIFY geometryPreferencesChanged)
 Q_PROPERTY(int keyboardScale READ keyboardScale WRITE setKeyboardScale NOTIFY geometryPreferencesChanged)
 Q_PROPERTY(bool highContrast MEMBER highContrast NOTIFY changed)
 Q_PROPERTY(int shortLabelScale MEMBER shortLabelScale NOTIFY changed)
 Q_PROPERTY(int longLabelScale MEMBER longLabelScale NOTIFY changed)
 Q_PROPERTY(bool rememberDesktop MEMBER rememberDesktop NOTIFY changed)
 Q_PROPERTY(QVariantMap layout MEMBER layout NOTIFY layoutChanged)
 Q_PROPERTY(QString layoutName MEMBER layoutName NOTIFY layoutChanged)
 Q_PROPERTY(QStringList layoutNames MEMBER layoutNames CONSTANT)
 Q_PROPERTY(QStringList menuLayoutNames MEMBER menuLayoutNames CONSTANT)
 Q_PROPERTY(bool shifted READ shifted NOTIFY changed)
 Q_PROPERTY(QStringList heldKeys READ heldKeys NOTIFY changed)
 Q_PROPERTY(QVariantMap modifierStates READ modifierStates NOTIFY changed)
 Q_PROPERTY(bool literal MEMBER literal NOTIFY changed)
 Q_PROPERTY(QString mappingJson READ mappingJson NOTIFY changed)
 Q_PROPERTY(QString scratchpad MEMBER scratchpad NOTIFY changed)
 Q_PROPERTY(QStringList suggestions MEMBER suggestions NOTIFY changed)
 Q_PROPERTY(bool learning MEMBER learning NOTIFY changed)
 Q_PROPERTY(bool hideKeyboardDuringMouseActions MEMBER hideKeyboardDuringMouseActions NOTIFY changed)
 Q_PROPERTY(bool keyboardTemporarilyHidden READ keyboardTemporarilyHidden NOTIFY changed)
 Q_PROPERTY(bool targeting MEMBER targeting NOTIFY changed)
 Q_PROPERTY(QString targetPrompt READ targetPrompt NOTIFY changed)
 Q_PROPERTY(QStringList heldPointerButtons READ heldPointerButtons NOTIFY changed)
 Q_PROPERTY(bool magneticCursor READ magneticCursor NOTIFY changed)
 Q_PROPERTY(int scrollSpeed MEMBER scrollSpeed NOTIFY changed)
 Q_PROPERTY(int scrollDeadzone MEMBER scrollDeadzone NOTIFY changed)
 Q_PROPERTY(int scrollAxis MEMBER scrollAxis NOTIFY changed)
 Q_PROPERTY(bool lookScrolling READ lookScrolling NOTIFY changed)
 Q_PROPERTY(bool magnifierEnabled READ isMagnifierEnabled WRITE setMagnifierEnabled NOTIFY changed)
 Q_PROPERTY(int magnifierZoom MEMBER magnifierZoom NOTIFY changed)
 Q_PROPERTY(bool progressiveZoom MEMBER progressiveZoom NOTIFY changed)
 Q_PROPERTY(int zoomDurationMs MEMBER zoomDurationMs NOTIFY changed)
 Q_PROPERTY(bool targetDwellEnabled MEMBER targetDwellEnabled NOTIFY changed)
 Q_PROPERTY(int targetStartDwellMs MEMBER targetStartDwellMs NOTIFY changed)
 Q_PROPERTY(int targetConfirmDwellMs MEMBER targetConfirmDwellMs NOTIFY changed)
 Q_PROPERTY(int targetDwellRadius MEMBER targetDwellRadius NOTIFY changed)
 Q_PROPERTY(bool zoomAnimating MEMBER zoomAnimating NOTIFY changed)
 Q_PROPERTY(bool capturePending MEMBER capturePending NOTIFY changed)
 Q_PROPERTY(bool magnifierReady MEMBER magnifierReady NOTIFY changed)
 Q_PROPERTY(int magnifierRevision MEMBER magnifierRevision NOTIFY changed)
public:
 Controller();~Controller()override;
 bool isMagnifierEnabled()const{return magnifierEnabled;}
 void setMagnifierEnabled(bool enabled);
 PortalInput *output(){return &portal;}
 bool ready()const{return nativeActive||portal.ready();}
 void configureInputMethod(std::function<bool(QString)> text,std::function<bool(QString,bool)> key);
 void inputContextChanged(bool active,bool sensitive);
 void invalidateInputContext();
 int keyboardScale()const{return scalePercent;}
 void setKeyboardScale(int scale);
 int restoreCorner()const{return recoveryCorner;}
 int restoreWidth()const{return recoveryWidth;}
 int restoreHeight()const{return recoveryHeight;}
 void setRestoreCorner(int corner);
 void setRestoreWidth(int width);
 void setRestoreHeight(int height);
 bool shifted()const{return runner.shifted();}
 QStringList heldKeys()const{return runner.heldKeys();}
 QVariantMap modifierStates()const{return runner.modifierStates();}
 Q_INVOKABLE QString modifierName(const QVariantList &commands)const{return ActionRunner::modifierName(commands);}
 QString mappingJson()const;
 QString targetPrompt()const{return zoomAnimating?"Zooming in…":magnifierReady?"Choose the precise target in the magnified image":pointer.prompt();}
 QImage magnifierImage()const{return magnifiedImage;}
 QStringList heldPointerButtons()const{return pointer.heldButtons();}
 bool keyboardTemporarilyHidden()const{return hideKeyboardDuringMouseActions&&pointer.busy();}
 bool magneticCursor()const{return pointer.magnetic();}
 bool lookScrolling()const{return pointer.lookScrolling();}
 bool isGazeEnabled()const{return gazeEnabled;}
 Q_INVOKABLE void setGazeEnabled(bool enabled);
 Q_INVOKABLE void trackGaze(double x,double y,bool overDesktop);
 void displayChanged();
 void setDisplaySize(double width,double height);
 int gazeSmoothingLevel()const{return smoothingLevel;}
 bool smoothWhenChangingGazeTarget()const{return smoothTargetChanges;}
 void setGazeSmoothingLevel(int level);
 void setSmoothWhenChangingGazeTarget(bool enabled);
 Q_INVOKABLE bool saveMappings(QString json);
 Q_INVOKABLE bool savePreferences();
 Q_INVOKABLE void resetPreferences();
 Q_INVOKABLE void connectDesktop(){portal.setRememberPermission(rememberDesktop);portal.connectPortal();}
 Q_INVOKABLE void forgetDesktopPermission(){portal.forgetPermission();rememberDesktop=false;emit changed();}
 Q_INVOKABLE void connectTobii(){if(!tobii.isRunning())tobii.start();}
 Q_INVOKABLE void togglePause();
 Q_INVOKABLE void activate(QString key);
 Q_INVOKABLE bool gamepadHeld(QString action)const{return gameController.held(action);}
 Q_INVOKABLE void joystick(int index,double x,double y,bool active);
 Q_INVOKABLE void clickCommand(QString key);
 Q_INVOKABLE double hover(QString key);
 Q_INVOKABLE double dwellAt(double x,double y,bool valid);
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
 void gazeEnabledChanged();
 void layoutChanged();
 void gazePoint(double x,double y,bool valid);
private:
 bool isSleepKey(const QString &key) const;
 bool performAction(QString action);
 void clearCapture();
 void updateMagnifierZoom();
 void updateText(QString text);
 void updateSuggestions();
 QVariantList mapped(const QVariantList &commands,bool &ok);
 Q_SLOT void receiveGaze(double x,double y,bool valid,qint64 sampledAt);
 GazeSmoothing gazeSmoothing;
 QSizeF gazeDisplaySize{1,1};
 int gazeIndicatorSize=18,gazeIndicatorOpacity=100;
 QString gazeIndicatorColor="#00bfff";
 int repeatIntervalMs=750;
 bool repeatPunctuationOnHover=false;
 bool repeatLettersOnHover=false;
 bool repeatBackspaceOnHover=false;
 bool repeatArrowsOnHover=false;
 bool cycleModifiersOnHover=false;
 bool hideKeyboardDuringMouseActions=false;
 bool gazeIndicatorMorph=false;
 int smoothingLevel=1;
 bool smoothTargetChanges=false;
 GameController gameController;
 int activeJoystick=-1;
 PortalInput portal;TobiiInput tobii;Dwell dwell;ActionRunner runner;PointerActions pointer;QElapsedTimer clock,gazeAge;
 ScreenshotPortal screenshot;
 Q_SLOT void receiveMagnifierFrame(QImage image);
 QTimer zoomTimer;
 QElapsedTimer zoomClock;
 QImage capturedImage;
 SpeechOutput speech;
 QImage magnifiedImage;
 QRect magnifierCrop;
 QSize captureSize;
 QPointF coarseTarget;
 bool magnifierEnabled=false,capturePending=false,magnifierReady=false;
 int magnifierRevision=0;
 int scrollSpeed=100,scrollDeadzone=50,scrollAxis=0;
 int magnifierZoom=4;
 bool progressiveZoom=false,zoomAnimating=false;
 int zoomDurationMs=1500,activeZoomDurationMs=1500,activeZoom=4;
 PointDwell pointDwell;
 bool targetDwellEnabled=true;
 int targetStartDwellMs=750,targetConfirmDwellMs=1000,targetDwellRadius=40;
 quint64 captureEpoch=0;
 QString portalStatus="Desktop control disconnected",tobiiStatus="Tobii ready to connect",actionStatus="Choose Authorize to enable desktop output";
 QString profileStatus;
 bool paused=false,literal=false,learning=false,targeting=false,gazeEnabled=false;
 int dwellMs=750;
 int scalePercent=100;
 int restoreOpacity=100;
 int recoveryCorner=2,recoveryWidth=64,recoveryHeight=64;
 bool highContrast=false;
 int shortLabelScale=150;
 int longLabelScale=100;
 bool rememberDesktop=false;
 bool nativeMode=false,nativeActive=false,sensitiveInput=false;
 qint64 lastGazeSample=0;
 QVariantMap layout;QString layoutName;QStringList layoutNames,menuLayoutNames,backStack;
 int suggestionOffset=0;
 QString scratchpad;QStringList suggestions,dictionary;QMap<QString,int> frequencies;
 QJsonObject mappingProfile;
};
