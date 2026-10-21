import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.desktop as Desktop
import org.kde.kirigami as Kirigami
Window {
 id: root
 objectName: "keyboardWindow"
 width: 1100; height: 370; visible: false
 title: "OptiKey Pro"
 color: minimizedRecovery?"transparent":controller.highContrast?"black":Kirigami.Theme.backgroundColor
 flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
 property string hovered: ""
 property real progress: 0
 property bool gazeEnabled: false
 property bool gazeCheck: false
 property bool minimizedRecovery:controller.layoutName.startsWith("minimized-off-screen-")
 property bool gazeValid: false
 property point gaze: Qt.point(-1,-1)
 property color dwellColor:controller.highContrast?"#ffff00":Kirigami.Theme.highlightColor
 // Geometry is supplied by the platform adapter; Wayland does not expose reliable global window positions.
 property real screenLeft: 0
 property real screenTop: 0
 property real screenWidth: 1100
 property real screenHeight: 900
 property real logicalX: 0
 property real logicalY: 530
 property real targetProgress:0
 property point targetPoint:Qt.point(-1,-1)
 property bool targetPointValid:false
 function keyAt(px,py) {
  for(let i=0;i<keys.count;++i){let item=keys.itemAt(i);if(!item)continue
   if(px>=item.x&&py>=item.y&&px<item.x+item.width&&py<item.y+item.height){
    if(item.modelData.kind==="SuggestionRow") {
     let n=Math.floor((px-item.x)/item.predictionWidth);return n<item.predictionCount&&n<controller.suggestions.length?"suggest:"+n:""
    }
    return "key:"+i
   }
  }
  return ""
 }
 Connections {
  target:controller
  function onSettingsRequested(){settings.showNormal();settings.raise();settings.requestActivate()}
  function onLayoutChanged(){root.hovered="";root.progress=0}
  function onGazePoint(x,y,valid){keyboardGazeIndicator.updateMotion(x,y,valid);root.gazeValid=valid;root.gaze=Qt.point(x,y)}
  function onChanged(){if(controller.keyboardTemporarilyHidden){root.hovered="";root.progress=0}if(!controller.targeting){root.targetProgress=0;root.targetPointValid=false}}
 }
 Timer {
  interval:25;running:!root.gazeCheck;repeat:true
  onTriggered:{
   let target=controller.keyboardTemporarilyHidden?"":root.hovered
   if(root.gazeEnabled){
    target=""
    if(root.gazeValid){
     let px=root.gaze.x*root.screenWidth,py=root.gaze.y*root.screenHeight
     let overPanel=root.visible&&px>=root.logicalX&&px<root.logicalX+root.width&&py>=root.logicalY&&py<root.logicalY+root.height
     controller.trackGaze(root.gaze.x,root.gaze.y,!overPanel&&!settings.visible)
     if(!controller.targeting&&root.visible)target=root.keyAt(px-root.logicalX,py-root.logicalY)
    }
   }
   if(controller.targeting){
    const point=root.gazeEnabled?root.gaze:Qt.point(targetPointer.mouseX/targetWindow.width,targetPointer.mouseY/targetWindow.height)
    const valid=root.gazeEnabled?root.gazeValid:targetPointer.containsMouse&&targetWindow.visible
    const cancelPoint=targetCancel.mapFromItem(targetWindow.contentItem,point.x*targetWindow.width,point.y*targetWindow.height)
    const overCancel=(!root.gazeEnabled&&targetCancel.hovered)||(valid&&cancelPoint.x>=0&&cancelPoint.y>=0&&cancelPoint.x<targetCancel.width&&cancelPoint.y<targetCancel.height)
    target=overCancel?"CancelTarget":""
    root.targetPoint=point;root.targetPointValid=valid&&!overCancel
    root.targetProgress=controller.dwellAt(point.x,point.y,valid&&!overCancel)
   }else{
    root.targetPointValid=false;root.targetProgress=controller.dwellAt(0,0,false)
   }
   root.progress=controller.hover(target)
  }
 }
 Repeater {
  id:keys
  model:controller.layout.keys || []
  Rectangle {
   id:key
   required property var modelData
   required property int index
   property string command:"key:"+index
   property string modifier:controller.modifierName(modelData.commands || [])
   property string modifierState:modifier?(controller.modifierStates[modifier] || ""):""
   property bool magnifierKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="MouseMagnifier"
   }
   property bool stateFilled:modifierState==="locked"||(magnifierKey&&controller.magnifierEnabled)||(sleepKey&&controller.paused)
   property real predictionWidth:2*root.width/controller.layout.cols
   property int predictionCount:Math.floor(modelData.width/2)
   property bool sleepKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="Sleep"&&!unavailableReason
   }
   property bool resumeKey:controller.paused&&sleepKey
   enabled:!controller.paused||sleepKey
   property string unavailableReason:modelData.unavailableReason || ""
   property string label:root.minimizedRecovery?"↗":controller.shifted&&modelData.shiftLabel?modelData.shiftLabel:modelData.label
   property real symbolMargin:controller.layout.symbolmargin!==undefined?Math.max(0,Number(controller.layout.symbolmargin)):12
   property color foreground:controller.highContrast?"white":Kirigami.Theme.textColor
   x:modelData.col*root.width/controller.layout.cols
   y:modelData.row*root.height/controller.layout.rows
   width:modelData.width*root.width/controller.layout.cols
   height:modelData.height*root.height/controller.layout.rows
   color:controller.highContrast?"black":Kirigami.Theme.backgroundColor
   opacity:(enabled?1:0.35)*(root.minimizedRecovery?controller.restoreOpacity/100:1)
   Kirigami.Theme.colorSet:Kirigami.Theme.Button
   Kirigami.Theme.inherit:false
   border.width:root.minimizedRecovery?0:1;border.color:Kirigami.Theme.disabledTextColor
   property bool active:enabled&&(root.gazeEnabled ? root.gazeValid&&root.keyAt(root.gaze.x*root.screenWidth-root.logicalX,root.gaze.y*root.screenHeight-root.logicalY)===command : root.hovered===command)
   Rectangle {anchors.fill:parent;color:Kirigami.Theme.highlightColor;opacity:key.active?0.18:0}
   Rectangle {
    anchors.fill:parent;anchors.margins:2
    visible:key.modifierState!==""||(key.magnifierKey&&controller.magnifierEnabled)||(key.sleepKey&&controller.paused)
    color:key.stateFilled?root.dwellColor:"transparent"
    opacity:key.stateFilled?0.25:1
    border.width:2;border.color:root.dwellColor
   }
   Column {
    anchors.centerIn:parent;spacing:4
    visible:key.modelData.kind!=="SuggestionRow"&&!key.unavailableReason
    Image {
     anchors.horizontalCenter:parent.horizontalCenter
     // NoIcon is the pack's X symbol, not an instruction to hide the image.
     visible:!root.minimizedRecovery&&key.modelData.symbol!==""
     width:Math.max(1,Math.min(key.width-2*key.symbolMargin,key.height-2*key.symbolMargin-(key.label?keyLabel.implicitHeight+4:0)))
     height:width
     source:visible?"image://symbols/"+key.modelData.symbol+"/"+encodeURIComponent(key.foreground):""
     sourceSize.width:width*root.screen.devicePixelRatio;sourceSize.height:height*root.screen.devicePixelRatio
     opacity:1
     fillMode:Image.PreserveAspectFit
    }
    Text {
     id:keyLabel
     visible:key.label!=="";text:key.label;color:key.foreground
     opacity:1
     width:Math.max(1,key.width-12)
     font.pixelSize:Math.max(16,Math.min(40,key.height*.32))*(text.length<=3?controller.shortLabelScale:controller.longLabelScale)/100
     minimumPixelSize:8;fontSizeMode:Text.HorizontalFit
     horizontalAlignment:Text.AlignHCenter;anchors.horizontalCenter:parent.horizontalCenter
    }
   }
   Text {
    anchors.fill:parent;anchors.margins:6
    visible:key.unavailableReason.length>0
    text:"Unavailable\nin Plasma mode";color:key.foreground
    font.pixelSize:16;minimumPixelSize:10;fontSizeMode:Text.Fit
    horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter
    wrapMode:Text.WordWrap
   }
   MouseArea {
    objectName:"keyMouseArea:"+key.index
    anchors.fill:parent;hoverEnabled:true;enabled:key.modelData.kind!=="SuggestionRow"
    onEntered:root.hovered=key.command
    onExited:if(root.hovered===key.command)root.hovered=""
    onClicked:controller.clickCommand(key.command)
   }
   Row {
    anchors.fill:parent;visible:key.modelData.kind==="SuggestionRow"
    Repeater {
     model:key.modelData.kind==="SuggestionRow"?key.predictionCount:0
     Rectangle {
      required property int index
      width:key.predictionWidth;height:key.height;color:"transparent";border.color:Kirigami.Theme.disabledTextColor
      Text {anchors.fill:parent;anchors.margins:4;text:index<controller.suggestions.length?controller.suggestions[index]:"";color:key.foreground;font.pixelSize:Math.min(24,key.height*.28)*(text.length<=3?controller.shortLabelScale:controller.longLabelScale)/100;fontSizeMode:Text.Fit;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter}
      MouseArea {objectName:"predictionMouseArea:"+index;anchors.fill:parent;hoverEnabled:true;onEntered:root.hovered="suggest:"+index;onExited:if(root.hovered==="suggest:"+index)root.hovered="";onClicked:controller.clickCommand("suggest:"+index)}
      Rectangle {anchors.bottom:parent.bottom;height:5;width:parent.width*(root.hovered==="suggest:"+index?root.progress:0);color:root.dwellColor}
     }
    }
   }
   Rectangle {anchors.bottom:parent.bottom;height:5;width:parent.width*(key.active?root.progress:0);color:root.dwellColor}
   Accessible.role:Accessible.Button
   Accessible.description:key.sleepKey?(controller.paused?"Pause on":"Pause off"):key.magnifierKey?(controller.magnifierEnabled?"Magnifier on":"Magnifier off"):key.modifier?(key.modifierState==="locked"?"Locked until selected again":key.modifierState==="latched"?"Latched for the next key":"Off. Select once to latch, twice to lock."):""
   Accessible.name:key.sleepKey?(controller.paused?"Resume input":"Pause input"):root.minimizedRecovery?"Restore keyboard":key.unavailableReason?key.unavailableReason:(key.label||key.modelData.symbol)
   Accessible.onPressAction:controller.clickCommand(key.command)
  }
 }
 Item {
  id:keyboardGazeIndicator;objectName:"keyboardGazeIndicator";visible:root.visible&&root.gazeEnabled&&!controller.targeting
  anchors.fill:parent;z:100;clip:true
  property real lastTime:0
  property point lastPoint:Qt.point(0,0)
  property point velocity:Qt.point(0,0)
  property bool hasVelocity:false
  property real stretch:0
  property real heading:0
  function resetMotion(){lastTime=0;hasVelocity=false;velocity=Qt.point(0,0);stretch=0;heading=0}
  onVisibleChanged:resetMotion()
  onWidthChanged:resetMotion()
  onHeightChanged:resetMotion()
  Connections {target:controller;function onChanged(){if(!controller.gazeIndicatorMorph)keyboardGazeIndicator.resetMotion()}}
  function updateMotion(x,y,valid){
   if(!valid||!visible||!controller.gazeIndicatorMorph){resetMotion();return}
   const now=Date.now(),point=Qt.point(x*root.screenWidth,y*root.screenHeight),dt=(now-lastTime)/1000
   if(lastTime>0&&dt>=0.004&&dt<=0.2){
    const vx=(point.x-lastPoint.x)/dt,vy=(point.y-lastPoint.y)/dt
    const speed=Math.sqrt(vx*vx+vy*vy)
    const acceleration=hasVelocity?Math.sqrt(Math.pow(vx-velocity.x,2)+Math.pow(vy-velocity.y,2))/dt:0
    stretch=speed>20?Math.min(1,acceleration/18000):0
    if(speed>20)heading=Math.atan2(vy,vx)*180/Math.PI
    velocity=Qt.point(vx,vy);hasVelocity=true
   }else{hasVelocity=false;stretch=0}
   lastTime=now;lastPoint=point
  }
  Rectangle {
   id:gazeMarker;objectName:"gazeMarker"
   property real localX:root.gaze.x*root.screenWidth-root.logicalX
   property real localY:root.gaze.y*root.screenHeight-root.logicalY
   visible:root.gazeValid&&localX>=0&&localX<root.width&&localY>=0&&localY<root.height
   x:localX-width/2;y:localY-height/2
   width:controller.gazeIndicatorSize;height:width;radius:width/2
   color:controller.gazeIndicatorColor;opacity:controller.gazeIndicatorOpacity/100
   antialiasing:true
   property real deformation:controller.gazeIndicatorMorph?keyboardGazeIndicator.stretch:0
   Behavior on deformation {NumberAnimation {duration:100}}
   transform:[
    Scale {origin.x:controller.gazeIndicatorSize/2;origin.y:controller.gazeIndicatorSize/2;xScale:1+0.8*gazeMarker.deformation;yScale:1/(1+0.8*gazeMarker.deformation)},
    Rotation {origin.x:controller.gazeIndicatorSize/2;origin.y:controller.gazeIndicatorSize/2;angle:keyboardGazeIndicator.heading}
   ]
  }
 }
 Window {
  id:settings;objectName:"settingsWindow";visible:false;width:Math.min(1000,root.screenWidth);height:Math.min(720,root.screenHeight-60);title:"OptiKey settings";color:Kirigami.Theme.backgroundColor
  minimumWidth:Math.min(760,root.screenWidth);minimumHeight:Math.min(500,root.screenHeight-60)
  property var categoryNames:["Status","Keyboard","Gaze & dwell","Magnifier","Typing & speech","Desktop access","Shortcuts"]
  property var categoryIcons:["dialog-information","input-keyboard","input-mouse","zoom-in","edit-text-frame-update","preferences-desktop","preferences-desktop-keyboard-shortcuts"]
  onClosing:controller.clickCommand("CancelTarget")
  component Tool:Desktop.Button {
   property string command:""
   focusPolicy:Qt.NoFocus
   onHoveredChanged:root.hovered=hovered?command:""
   onClicked:controller.clickCommand(command)
   Accessible.name:text
  }

  RowLayout {
   anchors.fill:parent;spacing:0
   Rectangle {
    Layout.preferredWidth:190;Layout.fillHeight:true
    color:Kirigami.Theme.backgroundColor
    Kirigami.Theme.colorSet:Kirigami.Theme.View
    Kirigami.Theme.inherit:false
    ListView {
     id:settingsCategory;objectName:"settingsCategory"
     anchors.fill:parent;anchors.margins:6;clip:true
     model:settings.categoryNames;currentIndex:0
     Accessible.name:"Settings categories"
     onCurrentIndexChanged:if(settingsScroll.contentItem)settingsScroll.contentItem.contentY=0
     delegate:Desktop.ItemDelegate {
      required property int index
      required property string modelData
      width:ListView.view.width;text:modelData
      icon.name:settings.categoryIcons[index]
      highlighted:ListView.isCurrentItem
      onClicked:settingsCategory.currentIndex=index
     }
    }
   }
   Kirigami.Separator {Layout.fillHeight:true}
   ColumnLayout {
    Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:16;spacing:12
    Kirigami.Heading {text:settings.categoryNames[settingsCategory.currentIndex];level:1;Layout.fillWidth:true}
    Desktop.Label {text:controller.paused?"Input is paused. Use the keyboard’s Sleep key to resume.":"Input is active.";wrapMode:Text.Wrap;Layout.fillWidth:true}
   Desktop.ScrollView {
    id:settingsScroll;Layout.fillWidth:true;Layout.fillHeight:true;contentWidth:availableWidth
    ColumnLayout {
     width:settingsScroll.availableWidth;spacing:12
     Desktop.GroupBox {
      title:settings.categoryNames[0];visible:settingsCategory.currentIndex==0;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       Desktop.Label {text:!root.gazeEnabled?"Gaze off":root.gazeValid?"Gaze: receiving":"Gaze: waiting for valid on-screen point";Layout.fillWidth:true;wrapMode:Text.Wrap}
       Desktop.Label {text:"Desktop control: "+controller.portalStatus;Layout.fillWidth:true;wrapMode:Text.Wrap}
       Desktop.Label {text:"Last action: "+controller.actionStatus;Layout.fillWidth:true;wrapMode:Text.Wrap}
       Desktop.Label {text:"Eye tracker: "+controller.tobiiStatus;Layout.fillWidth:true;wrapMode:Text.Wrap}
       Text {visible:text.length>0;text:controller.magneticCursor?"Cursor follows gaze":"";color:Kirigami.Theme.positiveTextColor}
       Text {visible:text.length>0;text:controller.lookScrolling?"Look to scroll active":"";color:Kirigami.Theme.positiveTextColor}
       Text {visible:text.length>0;text:controller.capturePending?"Waiting for desktop capture":controller.magnifierEnabled?"Magnifier on":"";color:Kirigami.Theme.positiveTextColor}
       Text {visible:text.length>0;text:controller.heldKeys.length?"Held: "+controller.heldKeys.join(", "):"";color:Kirigami.Theme.neutralTextColor}
       Text {visible:text.length>0;text:controller.heldPointerButtons.length?"Mouse held: "+controller.heldPointerButtons.join(", "):"";color:Kirigami.Theme.neutralTextColor}
       Tool {id:recover;text:"Home / recover";command:"Recover"}
       Desktop.Button {text:"Quit OptiKey";icon.name:"application-exit";onClicked:Qt.quit()}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[1];visible:settingsCategory.currentIndex==1;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       RowLayout {
        Desktop.Label {text:"Keyboard"}
        Desktop.ComboBox {
         model:includeHidden.checked?controller.layoutNames:controller.menuLayoutNames
         currentIndex:model.indexOf(controller.layoutName)
         displayText:currentIndex<0?controller.layoutName:currentText
         Layout.fillWidth:true;onActivated:controller.openLayout(currentText)
        }
       }
       Desktop.CheckBox {id:includeHidden;text:"Include layouts hidden by the XML pack"}
       Kirigami.Heading {text:"Appearance";level:3;Layout.topMargin:12}
       RowLayout {Desktop.Label {text:"Keyboard height: "+controller.keyboardScale+"%"} Desktop.Slider {from:50;to:150;stepSize:5;value:controller.keyboardScale;Layout.fillWidth:true;onMoved:controller.keyboardScale=value}}
       Desktop.CheckBox {text:"High contrast (white keys on black)";checked:controller.highContrast;onToggled:controller.highContrast=checked}
       RowLayout {Desktop.Label {text:"Short key label size: "+controller.shortLabelScale+"%"} Desktop.Slider {from:50;to:200;stepSize:5;value:controller.shortLabelScale;Layout.fillWidth:true;onMoved:controller.shortLabelScale=value}}
       RowLayout {Desktop.Label {text:"Long key label size: "+controller.longLabelScale+"%"} Desktop.Slider {from:50;to:200;stepSize:5;value:controller.longLabelScale;Layout.fillWidth:true;onMoved:controller.longLabelScale=value}}
       Desktop.Label {text:"Short labels have 3 characters or fewer. Text shrinks to fit each key.";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Desktop.CheckBox {text:"Temporarily hide keyboard during mouse actions";checked:controller.hideKeyboardDuringMouseActions;onToggled:controller.hideKeyboardDuringMouseActions=checked}
       Desktop.Label {text:"Restores the keyboard after the action finishes or is canceled. Target selection and Cancel remain available.";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Kirigami.Heading {text:"Restore button";level:3;Layout.topMargin:12}
       RowLayout {
        Desktop.Label {text:"Restore square corner"}
        Desktop.ComboBox {model:["Top left","Top right","Bottom left","Bottom right"];currentIndex:controller.restoreCorner;onActivated:controller.restoreCorner=currentIndex}
       }
       RowLayout {
        Desktop.Label {text:"Restore square size: "+controller.restoreSize+" px"}
        Desktop.Slider {from:40;to:128;stepSize:8;value:controller.restoreSize;Layout.fillWidth:true;onMoved:controller.restoreSize=value}
       }
       RowLayout {Desktop.Label {text:"Restore square opacity: "+controller.restoreOpacity+"%"} Desktop.Slider {from:10;to:100;stepSize:5;value:controller.restoreOpacity;Layout.fillWidth:true;onMoved:controller.restoreOpacity=value}}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[2];visible:settingsCategory.currentIndex==2;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       Desktop.CheckBox {text:"Enable gaze input";checked:root.gazeEnabled;enabled:!root.gazeCheck;focusPolicy:Qt.NoFocus;onToggled:{root.gazeEnabled=checked;controller.setGazeEnabled(checked)}}
       Tool {id:tobii;text:"Connect eye tracker";command:"Tobii"}
       Kirigami.Heading {text:"Gaze smoothing";level:3;Layout.topMargin:8}
       RowLayout {
        Desktop.Label {text:"Gaze smoothing"}
        Desktop.ComboBox {model:["0 — Off","1 — Low","2 — Medium","3 — High","4 — Extreme"];currentIndex:controller.gazeSmoothingLevel;onActivated:controller.gazeSmoothingLevel=currentIndex}
       }
       Desktop.CheckBox {text:"Smooth when changing gaze target";checked:controller.smoothWhenChangingGazeTarget;enabled:controller.gazeSmoothingLevel>0;onToggled:controller.smoothWhenChangingGazeTarget=checked}
       Desktop.Label {text:"Higher levels reduce jitter but respond more slowly.";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Kirigami.Heading {text:"Gaze indicator";level:3;Layout.topMargin:12}
       RowLayout {Desktop.Label {text:"Size: "+controller.gazeIndicatorSize+" px"} Desktop.Slider {from:4;to:120;stepSize:1;value:controller.gazeIndicatorSize;Layout.fillWidth:true;onMoved:controller.gazeIndicatorSize=value}}
       RowLayout {Desktop.Label {text:"Opacity: "+controller.gazeIndicatorOpacity+"%"} Desktop.Slider {from:5;to:100;stepSize:5;value:controller.gazeIndicatorOpacity;Layout.fillWidth:true;onMoved:controller.gazeIndicatorOpacity=value}}
       RowLayout {
        Desktop.Label {text:"Color"}
        Rectangle {width:24;height:24;radius:12;color:controller.gazeIndicatorColor;opacity:controller.gazeIndicatorOpacity/100}
        Desktop.TextField {text:controller.gazeIndicatorColor;placeholderText:"#RRGGBB";Accessible.name:"Gaze indicator color, hexadecimal RGB";validator:RegularExpressionValidator {regularExpression:/#[0-9a-fA-F]{6}/} onEditingFinished:{if(acceptableInput)controller.gazeIndicatorColor=text}}
       }
       Desktop.CheckBox {text:"Morph with gaze acceleration and direction";checked:controller.gazeIndicatorMorph;onToggled:controller.gazeIndicatorMorph=checked}
       Desktop.Label {text:"Stretches along gaze movement as acceleration increases, then returns to a solid circle.";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Kirigami.Heading {text:"Keyboard selection";level:3;Layout.topMargin:12}
       RowLayout {Desktop.Label {text:"Keyboard dwell: "+controller.dwellMs+" ms"} Desktop.Slider {from:250;to:3000;stepSize:50;value:controller.dwellMs;Layout.fillWidth:true;onMoved:controller.dwellMs=value}}

      }
     }
     Desktop.GroupBox {
      title:"Zoom behavior";visible:settingsCategory.currentIndex==3;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       RowLayout {Desktop.Label {text:"Magnifier zoom: "+controller.magnifierZoom+"×"} Desktop.Slider {from:2;to:8;stepSize:1;value:controller.magnifierZoom;Layout.fillWidth:true;onMoved:controller.magnifierZoom=value}}
       Desktop.CheckBox {text:"Progressive zoom";checked:controller.progressiveZoom;onToggled:controller.progressiveZoom=checked}
       RowLayout {Desktop.Label {text:"Zoom duration: "+(controller.zoomDurationMs/1000).toFixed(1)+" s"} Desktop.Slider {from:500;to:5000;stepSize:100;value:controller.zoomDurationMs;enabled:controller.progressiveZoom;Layout.fillWidth:true;onMoved:controller.zoomDurationMs=value}}
       Kirigami.Heading {text:"Target selection";level:3;Layout.topMargin:12}
       Desktop.CheckBox {text:"Select pointer targets by dwell";checked:controller.targetDwellEnabled;onToggled:controller.targetDwellEnabled=checked}
       RowLayout {Desktop.Label {text:"Start zoom / target dwell: "+controller.targetStartDwellMs+" ms"} Desktop.Slider {from:250;to:3000;stepSize:50;value:controller.targetStartDwellMs;enabled:controller.targetDwellEnabled;Layout.fillWidth:true;onMoved:controller.targetStartDwellMs=value}}
       RowLayout {Desktop.Label {text:"Confirm zoomed target dwell: "+controller.targetConfirmDwellMs+" ms"} Desktop.Slider {from:250;to:3000;stepSize:50;value:controller.targetConfirmDwellMs;enabled:controller.targetDwellEnabled;Layout.fillWidth:true;onMoved:controller.targetConfirmDwellMs=value}}
       RowLayout {Desktop.Label {text:"Target dwell movement tolerance: "+controller.targetDwellRadius+" px"} Desktop.Slider {from:10;to:100;stepSize:5;value:controller.targetDwellRadius;enabled:controller.targetDwellEnabled;Layout.fillWidth:true;onMoved:controller.targetDwellRadius=value}}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[4];visible:settingsCategory.currentIndex==4;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       Desktop.CheckBox {text:"Literal keys (bypass platform mappings)";checked:controller.literal;onToggled:controller.literal=checked}
       Desktop.CheckBox {text:"Learn word frequencies locally";checked:controller.learning;onToggled:controller.learning=checked}
       Desktop.Button {text:"Reset learned words";onClicked:controller.resetLearning()}
       Kirigami.Heading {text:"Speech";level:3;Layout.topMargin:8}
       Flow {Layout.fillWidth:true;spacing:8
        Tool {id:speakButton;text:"Speak";command:"Speak"}
        Tool {id:stopSpeechButton;text:"Stop speech";command:"StopSpeech"}
       }
       Desktop.Label {text:"Scratchpad (text typed by OptiKey only)"}
       Desktop.TextArea {text:controller.scratchpad;readOnly:true;Layout.fillWidth:true;Layout.preferredHeight:70;wrapMode:Text.Wrap}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[5];visible:settingsCategory.currentIndex==5;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       Tool {id:authorize;text:"Connect desktop control";command:"Connect"}
       Desktop.CheckBox {text:"Remember desktop-control permission, if supported";checked:controller.rememberDesktop;onToggled:controller.rememberDesktop=checked}
       Desktop.Label {text:"When saved, this also connects keyboard and mouse control automatically at startup. The desktop may still ask for permission.";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Desktop.Button {text:"Disconnect and forget desktop permission";onClicked:controller.forgetDesktopPermission()}
       Desktop.Label {text:"Screenshot access is separate. Approve the first magnifier capture; supported desktops remember that approval for OptiKey until it is revoked.";wrapMode:Text.Wrap;Layout.fillWidth:true}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[6];visible:settingsCategory.currentIndex==6;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:10
       Desktop.Label {text:"Linux shortcut mappings — source XML stays unchanged"}
       Desktop.Label {text:"Each override has match (the exact signature shown in Settings → Status), description, and commands [{tag, text}]. Unmapped Windows combinations require an override or Literal keys.";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Desktop.ScrollView {Layout.fillWidth:true;Layout.preferredHeight:180;Desktop.TextArea {id:mappingEditor;text:controller.mappingJson;wrapMode:Text.Wrap;font.family:"monospace";selectByMouse:true}}
       Desktop.Button {text:"Save mappings";onClicked:controller.saveMappings(mappingEditor.text)}
       Desktop.Label {text:controller.actionStatus;wrapMode:Text.Wrap;Layout.fillWidth:true}
      }
     }
    }
   }
   Desktop.Label {text:"Changes apply immediately. Save preferences to keep them after restart.";wrapMode:Text.Wrap;Layout.fillWidth:true}
   Desktop.Label {text:controller.profileStatus;visible:text.length>0;wrapMode:Text.Wrap;Layout.fillWidth:true}
   Flow {
    Layout.fillWidth:true;spacing:8
    Desktop.Button {text:"Save preferences";icon.name:"document-save";ToolTip.text:"Save preferences and startup keyboard";ToolTip.visible:hovered;onClicked:controller.savePreferences()}
    Desktop.Button {text:"Reset preferences";icon.name:"edit-undo";onClicked:controller.resetPreferences()}
    Desktop.Button {text:"Close";icon.name:"window-close";onClicked:settings.close()}
   }
  }
 }
 }

 Window {
  id:targetWindow;objectName:"targetWindow";visible:controller.targeting&&!controller.capturePending;width:root.screenWidth;height:root.screenHeight
  color:"transparent";flags:Qt.FramelessWindowHint|Qt.WindowDoesNotAcceptFocus|Qt.WindowStaysOnTopHint
  Image {anchors.fill:parent;visible:controller.magnifierReady;source:visible?"image://magnifier/"+controller.magnifierRevision:"";cache:false;fillMode:Image.Stretch}
  Rectangle {
   anchors.horizontalCenter:parent.horizontalCenter;y:90
   visible:!controller.magnifierEnabled
   width:targetLabel.implicitWidth+32;height:targetLabel.implicitHeight+20
   color:Kirigami.Theme.backgroundColor;radius:4
   Text {id:targetLabel;anchors.centerIn:parent;text:controller.targetPrompt;font.pixelSize:24;color:Kirigami.Theme.textColor}
  }
  Button {
   id:targetCancel;anchors.horizontalCenter:parent.horizontalCenter;y:145;text:"Cancel";focusPolicy:Qt.NoFocus;z:2
   onHoveredChanged:root.hovered=hovered?"CancelTarget":""
   onClicked:controller.clickCommand("CancelTarget")
   Rectangle {anchors.bottom:parent.bottom;height:5;width:parent.width*root.progress;color:Kirigami.Theme.highlightColor}
  }
  MouseArea {id:targetPointer;objectName:"targetPointer";anchors.fill:parent;hoverEnabled:true;onClicked:mouse=>controller.targetAt(mouse.x/width,mouse.y/height)}
  Canvas {
   visible:root.targetPointValid&&!controller.zoomAnimating
   x:root.targetPoint.x*targetWindow.width-width/2;y:root.targetPoint.y*targetWindow.height-height/2
   width:56;height:56
   property real progress:root.targetProgress
   property color ringColor:root.dwellColor
   onProgressChanged:requestPaint()
   onRingColorChanged:requestPaint()
   onVisibleChanged:requestPaint()
   onPaint:{
    const ctx=getContext("2d");ctx.clearRect(0,0,width,height)
    ctx.lineWidth=3;ctx.strokeStyle=ringColor;ctx.globalAlpha=.35
    ctx.beginPath();ctx.arc(width/2,height/2,23,0,2*Math.PI);ctx.stroke()
    ctx.globalAlpha=1;ctx.lineWidth=5;ctx.beginPath();ctx.arc(width/2,height/2,23,-Math.PI/2,-Math.PI/2+2*Math.PI*progress);ctx.stroke()
   }
  }
 }
}
