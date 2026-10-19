import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
Window {
 id: root
 objectName: "keyboardWindow"
 width: 1100; height: 370; visible: false
 title: "OptiKey Pro"
 color: controller.layout.backgroundcolor || "white"
 flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
 property string hovered: ""
 property real progress: 0
 property bool gazeEnabled: false
 property bool gazeValid: false
 property point gaze: Qt.point(-1,-1)
 // Geometry is supplied by the platform adapter; Wayland does not expose reliable global window positions.
 property real screenLeft: 0
 property real screenTop: 0
 property real screenWidth: 1100
 property real screenHeight: 900
 property real logicalX: 0
 property real logicalY: 530
 property string targetCell: ""
 property real targetSince: 0
 property string targetPrompt: controller.targetPrompt
 onTargetPromptChanged: {targetCell="";targetSince=0}
 function keyAt(px,py) {
  for(let i=0;i<keys.count;++i){let item=keys.itemAt(i);if(!item)continue
   if(px>=item.x&&py>=item.y&&px<item.x+item.width&&py<item.y+item.height){
    if(item.modelData.kind==="SuggestionRow") {
     let n=Math.floor((px-item.x)/(item.width/8));return n<controller.suggestions.length?"suggest:"+n:""
    }
    return "key:"+i
   }
  }
  return ""
 }
 Connections {
  target:controller
  function onSettingsRequested(){settings.show();settings.requestActivate()}
  function onLayoutChanged(){root.hovered="";root.progress=0}
  function onGazePoint(x,y,valid){root.gazeValid=valid;root.gaze=Qt.point(x,y)}
  function onChanged(){if(!controller.targeting){root.targetCell="";root.targetSince=0}}
 }
 Timer {
  interval:25;running:true;repeat:true
  onTriggered:{
   let target=root.hovered
   if(root.gazeEnabled){
    target=""
    if(root.gazeValid){
     let px=root.gaze.x*root.screenWidth,py=root.gaze.y*root.screenHeight
     let barY=controller.layout.position==="Top"?root.screenHeight-controls.height:0
     let overPanel=px>=root.logicalX&&px<root.logicalX+root.width&&py>=root.logicalY&&py<root.logicalY+root.height
     controller.trackGaze(root.gaze.x,root.gaze.y,!overPanel&&!(py>=barY&&py<barY+controls.height)&&!settings.visible)
     if(controller.targeting){
      let cancelPoint=targetCancel.mapFromItem(targetWindow.contentItem,px,py)
      if(cancelPoint.x>=0&&cancelPoint.y>=0&&cancelPoint.x<targetCancel.width&&cancelPoint.y<targetCancel.height){
       target="CancelTarget";root.targetCell="";root.targetSince=0
      }else{
       let cell=Math.floor(px/40)+":"+Math.floor(py/40), now=Date.now()
       if(cell!==root.targetCell){root.targetCell=cell;root.targetSince=now}
       else if(now-root.targetSince>=controller.dwellMs){root.targetSince=now+100000;controller.targetAt(root.gaze.x,root.gaze.y)}
      }
     }else{
      let barTop=controller.layout.position==="Top"?root.screenHeight-controls.height:0
      if(py>=barTop&&py<barTop+controls.height)target=controls.commandAt(px,py-barTop)
      else target=root.keyAt(px-root.logicalX,py-root.logicalY)
     }
    }else {root.targetCell="";root.targetSince=0}
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
   property string unavailableReason:modelData.unavailableReason || ""
   property string label:controller.shifted&&modelData.shiftLabel?modelData.shiftLabel:modelData.label
   property real symbolMargin:controller.layout.symbolmargin!==undefined?Math.max(0,Number(controller.layout.symbolmargin)):12
   property color foreground:controller.highContrast?"white":modelData.style.foregroundcolor
   x:modelData.col*root.width/controller.layout.cols
   y:modelData.row*root.height/controller.layout.rows
   width:modelData.width*root.width/controller.layout.cols
   height:modelData.height*root.height/controller.layout.rows
   color:controller.highContrast?"black":modelData.style.backgroundcolor
   opacity:controller.highContrast?1:Number(modelData.style.opacity)
   border.width:1;border.color:"#d4d9df"
   property bool active:root.gazeEnabled ? root.gazeValid&&root.keyAt(root.gaze.x*root.screenWidth-root.logicalX,root.gaze.y*root.screenHeight-root.logicalY)===command : root.hovered===command
   Rectangle {anchors.fill:parent;color:"#2387b8";opacity:key.active?0.18:0}
   Column {
    anchors.centerIn:parent;spacing:4
    visible:key.modelData.kind!=="SuggestionRow"&&!key.unavailableReason
    Image {
     anchors.horizontalCenter:parent.horizontalCenter
     visible:key.modelData.symbol!==""&&key.modelData.symbol!=="NoIcon"
     width:Math.max(1,Math.min(key.width-2*key.symbolMargin,key.height-2*key.symbolMargin-(key.label?30:0)))
     height:width
     source:visible?"image://symbols/"+key.modelData.symbol+"/"+encodeURIComponent(key.foreground):""
     sourceSize.width:width*root.screen.devicePixelRatio;sourceSize.height:height*root.screen.devicePixelRatio
     opacity:controller.highContrast?1:Number(key.modelData.style.foregroundopacity)
     fillMode:Image.PreserveAspectFit
    }
    Text {
     visible:key.label!=="";text:key.label;color:key.foreground
     opacity:controller.highContrast?1:Number(key.modelData.style.foregroundopacity)
     font.pixelSize:Math.max(16,Math.min(40,key.height*.32));anchors.horizontalCenter:parent.horizontalCenter
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
   ToolTip.visible:key.active&&key.unavailableReason.length>0
   ToolTip.text:key.unavailableReason
   ToolTip.delay:400
   MouseArea {
    anchors.fill:parent;hoverEnabled:true;enabled:key.modelData.kind!=="SuggestionRow"
    onEntered:root.hovered=key.command
    onExited:if(root.hovered===key.command)root.hovered=""
    onClicked:controller.clickCommand(key.command)
   }
   Row {
    anchors.fill:parent;visible:key.modelData.kind==="SuggestionRow"
    Repeater {
     model:8
     Rectangle {
      required property int index
      width:key.width/8;height:key.height;color:"transparent";border.color:"#d4d9df"
      Text {anchors.fill:parent;anchors.margins:4;text:index<controller.suggestions.length?controller.suggestions[index]:"";color:key.foreground;font.pixelSize:Math.min(24,key.height*.28);fontSizeMode:Text.Fit;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter}
      MouseArea {anchors.fill:parent;hoverEnabled:true;onEntered:root.hovered="suggest:"+index;onExited:root.hovered="";onClicked:controller.clickCommand("suggest:"+index)}
      Rectangle {anchors.bottom:parent.bottom;height:5;width:parent.width*(root.hovered==="suggest:"+index?root.progress:0);color:"#168e86"}
     }
    }
   }
   Rectangle {anchors.bottom:parent.bottom;height:5;width:parent.width*(key.active?root.progress:0);color:"#168e86"}
   Accessible.role:Accessible.Button
   Accessible.name:key.unavailableReason?key.unavailableReason:(key.label||key.modelData.symbol)
   Accessible.onPressAction:controller.clickCommand(key.command)
  }
 }
 Rectangle {visible:root.gazeEnabled&&root.gazeValid&&!controller.targeting;x:root.gaze.x*root.screenWidth-root.logicalX-7;y:root.gaze.y*root.screenHeight-root.logicalY-7;width:14;height:14;radius:7;color:"transparent";border.width:2;border.color:"#008e83"}
 Window {
  id:controls;objectName:"controlWindow";visible:false;width:1100;height:84;color:"#101923"
  title:"OptiKey controls";flags:Qt.FramelessWindowHint|Qt.WindowDoesNotAcceptFocus|Qt.WindowStaysOnTopHint
  function commandAt(x,y){
   for(let item of [authorize,tobii,pause,recover,openSettings,speakButton,stopSpeechButton]){let p=item.mapFromItem(controls.contentItem,x,y);if(p.x>=0&&p.y>=0&&p.x<item.width&&p.y<item.height)return item.command}
   return ""
  }
  component Tool:Button {
   property string command:""
   focusPolicy:Qt.NoFocus
   onHoveredChanged:root.hovered=hovered?command:""
   onClicked:controller.clickCommand(command)
   Accessible.name:text
  }
  ColumnLayout {
   anchors.fill:parent;anchors.margins:6;spacing:2
   RowLayout {
    Text {text:"OptiKey Pro";color:"white";font.bold:true;font.pixelSize:18}
    Tool {id:authorize;text:"Authorize";command:"Connect"}
    Tool {id:tobii;text:"Tobii";command:"Tobii"}
    Tool {id:pause;text:controller.paused?"Resume":"Pause";command:"Pause"}
    Tool {id:recover;text:"Home / recover";command:"Recover"}
    CheckBox {text:"Gaze";palette.windowText:"white";palette.text:"white";focusPolicy:Qt.NoFocus;onToggled:{root.gazeEnabled=checked;controller.setGazeEnabled(checked)}}
    Text {text:controller.magneticCursor?"Cursor follows gaze":"";color:"#65d7c2"}
    Text {text:controller.lookScrolling?"Look to scroll active":"";color:"#65d7c2"}
    Text {text:controller.capturePending?"Waiting for desktop capture":controller.magnifierEnabled?"Magnifier on":"";color:"#65d7c2"}
    Tool {id:openSettings;text:"Settings";command:"Settings"}
    Tool {id:speakButton;text:"Speak";command:"Speak"}
    Tool {id:stopSpeechButton;text:"Stop speech";command:"StopSpeech"}
    Item {Layout.fillWidth:true}
    Text {text:controller.heldKeys.length?"Held: "+controller.heldKeys.join(", "):"";color:"#ffd49a"}
    Text {text:controller.heldPointerButtons.length?"Mouse held: "+controller.heldPointerButtons.join(", "):"";color:"#ffd49a"}
    Button {text:"Close";focusPolicy:Qt.NoFocus;onClicked:Qt.quit()}
   }
   RowLayout {
    Text {text:controller.portalStatus;color:controller.ready?"#65d7c2":"#ffd49a";font.pixelSize:12}
    Text {text:controller.actionStatus;color:"#d2dfec";font.pixelSize:12;Layout.fillWidth:true;elide:Text.ElideRight}
    Text {text:controller.tobiiStatus;color:"#d2dfec";font.pixelSize:12}
   }
  }
 }
 Window {
  id:settings;objectName:"settingsWindow";visible:false;width:Math.min(860,root.screenWidth);height:Math.min(850,root.screenHeight-60);title:"OptiKey settings";color:"#f4f6f9"
  onClosing:controller.clickCommand("CancelTarget")
  ScrollView {
   id:settingsScroll;anchors.fill:parent;anchors.margins:16;contentWidth:availableWidth
  ColumnLayout {
   width:settingsScroll.availableWidth;spacing:8
   Label {text:"Input is paused while you edit settings. Resume from the control bar.";wrapMode:Text.Wrap;Layout.fillWidth:true}
   RowLayout {
    Label {text:"Keyboard"}
    ComboBox {
     model:includeHidden.checked?controller.layoutNames:controller.menuLayoutNames
     currentIndex:model.indexOf(controller.layoutName)
     displayText:currentIndex<0?controller.layoutName:currentText
     Layout.fillWidth:true;onActivated:controller.openLayout(currentText)
    }
   }
   CheckBox {id:includeHidden;text:"Include layouts hidden by the XML pack"}
   RowLayout {Label {text:"Dwell: "+controller.dwellMs+" ms"} Slider {from:250;to:3000;stepSize:50;value:controller.dwellMs;Layout.fillWidth:true;onMoved:controller.dwellMs=value}}
   RowLayout {Label {text:"Keyboard height: "+controller.keyboardScale+"%"} Slider {from:50;to:150;stepSize:5;value:controller.keyboardScale;Layout.fillWidth:true;onMoved:controller.keyboardScale=value}}
   CheckBox {text:"High contrast (overrides imported key colors)";checked:controller.highContrast;onToggled:controller.highContrast=checked}
   CheckBox {text:"Literal keys (bypass platform mappings)";checked:controller.literal;onToggled:controller.literal=checked}
   CheckBox {text:"Learn word frequencies locally";checked:controller.learning;onToggled:controller.learning=checked}
   CheckBox {text:"Remember desktop-control permission, if supported";checked:controller.rememberDesktop;onToggled:controller.rememberDesktop=checked}
   Button {text:"Disconnect and forget desktop permission";onClicked:controller.forgetDesktopPermission()}
   RowLayout {
    Button {text:"Save preferences and startup keyboard";onClicked:controller.savePreferences()}
    Button {text:"Reset preferences";onClicked:controller.resetPreferences()}
   }
   Label {text:controller.profileStatus;visible:text.length>0;wrapMode:Text.Wrap;Layout.fillWidth:true}
   Button {text:"Reset learned words";onClicked:controller.resetLearning()}
   Label {text:"Scratchpad (text typed by OptiKey only)"}
   TextArea {text:controller.scratchpad;readOnly:true;Layout.fillWidth:true;Layout.preferredHeight:70;wrapMode:Text.Wrap}
   Label {text:"Linux shortcut mappings — source XML stays unchanged"}
   Label {text:"Each override has match (the exact signature shown in the control bar), description, and commands [{tag, text}]. Unmapped Windows combinations require an override or Literal keys.";wrapMode:Text.Wrap;Layout.fillWidth:true}
   ScrollView {Layout.fillWidth:true;Layout.preferredHeight:180;TextArea {id:mappingEditor;text:controller.mappingJson;wrapMode:Text.Wrap;font.family:"monospace";selectByMouse:true}}
   RowLayout {Button {text:"Save mappings";onClicked:controller.saveMappings(mappingEditor.text)} Label {text:controller.actionStatus;Layout.fillWidth:true;elide:Text.ElideRight} Button {text:"Close settings";onClicked:settings.close()}}
  }
  }
 }
 Window {
  id:targetWindow;objectName:"targetWindow";visible:controller.targeting&&!controller.capturePending;width:root.screenWidth;height:root.screenHeight
  color:"#1800b7a0";flags:Qt.FramelessWindowHint|Qt.WindowDoesNotAcceptFocus|Qt.WindowStaysOnTopHint
  Image {anchors.fill:parent;visible:controller.magnifierReady;source:visible?"image://magnifier/"+controller.magnifierRevision:"";cache:false;fillMode:Image.Stretch}
  Text {anchors.horizontalCenter:parent.horizontalCenter;y:100;text:controller.targetPrompt+" — click, or hold your gaze";font.pixelSize:24;color:"#103d37"}
  Button {
   id:targetCancel;anchors.horizontalCenter:parent.horizontalCenter;y:145;text:"Cancel";focusPolicy:Qt.NoFocus;z:2
   onHoveredChanged:root.hovered=hovered?"CancelTarget":""
   onClicked:controller.clickCommand("CancelTarget")
   Rectangle {anchors.bottom:parent.bottom;height:5;width:parent.width*root.progress;color:"#168e86"}
  }
  MouseArea {anchors.fill:parent;onClicked:mouse=>controller.targetAt(mouse.x/width,mouse.y/height)}
  Rectangle {visible:root.gazeEnabled&&root.gazeValid;x:root.gaze.x*parent.width-20;y:root.gaze.y*parent.height-20;width:40;height:40;radius:20;color:"transparent";border.color:"#00776f";border.width:3}
 }
}
