import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import org.kde.desktop as Desktop
import org.kde.kirigami as Kirigami
Window {
 id: root
 objectName: "keyboardWindow"
 width: 1100; height: 370; visible: false
 title: "GazeGrid"
 // Keep the native surface translucent for its entire lifetime.
 color:"transparent"
 flags: Qt.FramelessWindowHint | Qt.WindowDoesNotAcceptFocus | Qt.WindowStaysOnTopHint
 property string hovered: ""
 property bool canvasDragging:false
 property point canvasPoint:Qt.point(0,0)
 // Keep coordinates current before entering edit mode as well.
 HoverHandler {id:canvasHover}
 property var dwellStyleIds: ["bar","top-bar","fill","outline","ring","pie","collapse"]
 property var dwellStyleNames: ["Bottom bar","Top bar","Rising fill","Outline","Ring","Pie","Collapse"]
 property real progress: 0
 property var overlapProgress: ({})
 readonly property bool overlapEnabled:controller.layout.layoutMode==="canvas"&&controller.layout.activateoverlappingkeys===true&&!controller.sizeMoveMode
 readonly property var overlapTargets: {
  const revision=hitTestRevision
  if(!overlapEnabled||!root.visible||controller.targeting||controller.keyboardTemporarilyHidden)return []
  if(!root.usingGaze&&!canvasHover.hovered)return []
  const point=root.usingGaze?Qt.point(gaze.x*screenWidth-logicalX,gaze.y*screenHeight-logicalY):canvasHover.point.position
  return keysAt(point.x,point.y)
 }
 property bool gazeEnabled: controller.gazeEnabled
 property bool gazeCheck: false
 property bool minimizedRecovery:controller.layoutName.startsWith("minimized-off-screen-")
 property bool usingGaze: gazeEnabled&&gazeValid
 onUsingGazeChanged:{
  // Require fresh pointer motion after a gaze transition; stale hover must not
  // deflect a joystick again immediately after tracking loss.
  if(root.hovered.startsWith("key:")){
   const item=keys.itemAt(Number(root.hovered.substring(4)))
   if(item&&item.modelData.kind==="Joystick")root.hovered=""
  }
  controller.joystick(-1,0,0,false);controller.hover("");controller.dwellAt(0,0,false);root.progress=0;root.targetProgress=0}
 property bool gazeValid: false
 property point gaze: Qt.point(-1,-1)
 readonly property var themeIds:["system","midnight","ocean","forest","paper","high-contrast"]
 readonly property var themeNames:["System","Midnight","Ocean","Forest","Warm Paper","High Contrast"]
 readonly property var themePalettes:({
  "high-contrast":{background:"#000000",key:"#000000",text:"#ffffff",border:"#ffffff",accent:"#ffff00"},
  midnight:{background:"#111827",key:"#1f2937",text:"#f3f4f6",border:"#64748b",accent:"#c4b5fd"},
  ocean:{background:"#082f49",key:"#0c4a6e",text:"#f0f9ff",border:"#38bdf8",accent:"#67e8f9"},
  forest:{background:"#102b24",key:"#1b4332",text:"#f0fdf4",border:"#6b9e80",accent:"#a7f3d0"},
  paper:{background:"#e9dfcd",key:"#fff8eb",text:"#3d3026",border:"#95816b",accent:"#92400e"}
 })
 readonly property var keyboardPalette:themePalettes[controller.keyboardTheme]||null
 readonly property color keyboardBackground:keyboardPalette?keyboardPalette.background:Kirigami.Theme.backgroundColor
 readonly property color keyboardForeground:keyboardPalette?keyboardPalette.text:Kirigami.Theme.textColor
 readonly property color keyboardBorder:keyboardPalette?keyboardPalette.border:Kirigami.Theme.disabledTextColor
 property color dwellColor:keyboardPalette?keyboardPalette.accent:Kirigami.Theme.highlightColor
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
 // Repeater lifecycle changes must invalidate the shared hit even when the
 // replacement layout has the same number of keys.
 readonly property bool transparentBackground: (controller.layout.transparentbackground === "true") || root.minimizedRecovery || (controller.layout.keys||[]).some(k=>root.styleOpacity(k.style,"opacity")<1)
 readonly property bool backgroundClickThrough: transparentBackground && (controller.layout.backgroundclickthrough === "true") && !controller.sizeMoveMode
 onBackgroundClickThroughChanged:Qt.callLater(updateInputMask)
 onWidthChanged:Qt.callLater(updateInputMask)
 onHeightChanged:Qt.callLater(updateInputMask)
 function updateInputMask() {
  let rectangles=[]
  for(let i=0;i<keys.count;++i){let key=keys.itemAt(i);if(key&&key.visible)rectangles.push(Qt.rect(key.x,key.y,key.width,key.height))}
  controller.updateKeyboardMask(root,rectangles,backgroundClickThrough)
 }
 Component.onCompleted:Qt.callLater(updateInputMask)
 property int hitTestRevision: 0
 onHitTestRevisionChanged:Qt.callLater(updateInputMask)
 readonly property string gazeTarget: {
  const revision=hitTestRevision
  return usingGaze ? keyAt(gaze.x*screenWidth-logicalX,gaze.y*screenHeight-logicalY) : ""
 }
 function styleOpacity(style,name) {
  const value=style&&style[name]!==undefined?Number(style[name]):1
  return isFinite(value)?Math.max(0,Math.min(1,value)):1
 }
 function keyAt(px,py) {
  for(let i=keys.count-1;i>=0;--i){let item=keys.itemAt(i);if(!item)continue
   if(px>=item.x&&py>=item.y&&px<item.x+item.width&&py<item.y+item.height){
    if(controller.sizeMoveMode)return "key:"+i
    if(item.modelData.kind==="SuggestionRow") {
     let n=Math.floor((px-item.x)/item.predictionWidth);return n<item.predictionCount&&n<controller.suggestions.length&&controller.suggestions[n].trim().length>0?"suggest:"+n:""
    }
    if(item.modelData.kind==="Scratchpad")return ""
    return "key:"+i
   }
  }
  return ""
 }
 function keysAt(px,py) {
  // Joysticks and prediction slots retain their exclusive interaction model.
  const top=keyAt(px,py)
  if(!top.startsWith("key:"))return []
  const item=keys.itemAt(Number(top.substring(4)))
  if(!item||item.modelData.kind!=="DynamicKey")return []
  let targets=[]
  for(let i=0;i<keys.count;++i){const key=keys.itemAt(i)
   if(key&&key.modelData.kind==="DynamicKey"&&px>=key.x&&py>=key.y&&px<key.x+key.width&&py<key.y+key.height)targets.push("key:"+i)
  }
  return targets
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
   if(controller.sizeMoveMode){
    const point=root.usingGaze?Qt.point(root.gaze.x*root.screenWidth-root.logicalX,root.gaze.y*root.screenHeight-root.logicalY):canvasHover.point.position
    const valid=root.visible&&!controller.keyboardTemporarilyHidden&&(root.usingGaze?root.gazeValid:canvasHover.hovered)&&!root.canvasDragging
    root.progress=controller.canvasEditAt(point.x/root.width,point.y/root.height,valid)
    root.canvasPoint=point
    return
   }
   let target=controller.keyboardTemporarilyHidden?"":root.hovered
   if(root.usingGaze){
    target=""
    if(root.gazeValid){
     let px=root.gaze.x*root.screenWidth,py=root.gaze.y*root.screenHeight
     let overPanel=root.visible&&px>=root.logicalX&&px<root.logicalX+root.width&&py>=root.logicalY&&py<root.logicalY+root.height
     controller.trackGaze(root.gaze.x,root.gaze.y,!overPanel)
     if(!controller.targeting&&root.visible)target=root.gazeTarget
    }
   }
   if(controller.targeting){
    const point=root.usingGaze?root.gaze:Qt.point(targetPointer.mouseX/targetWindow.width,targetPointer.mouseY/targetWindow.height)
    const valid=root.usingGaze?root.gazeValid:targetPointer.containsMouse&&targetWindow.visible
    const cancelPoint=targetCancel.mapFromItem(targetWindow.contentItem,point.x*targetWindow.width,point.y*targetWindow.height)
    const overCancel=(!root.usingGaze&&targetCancel.hovered)||(valid&&cancelPoint.x>=0&&cancelPoint.y>=0&&cancelPoint.x<targetCancel.width&&cancelPoint.y<targetCancel.height)
    target=overCancel?"CancelTarget":""
    root.targetPoint=point;root.targetPointValid=valid&&!overCancel
    root.targetProgress=controller.dwellAt(point.x,point.y,valid&&!overCancel)
   }else{
    root.targetPointValid=false;root.targetProgress=controller.dwellAt(0,0,false)
   }
   let joystickIndex=-1
   if(root.visible&&!controller.targeting&&!controller.keyboardTemporarilyHidden&&target.startsWith("key:")){
    let i=Number(target.substring(4)),item=keys.itemAt(i)
    if(item&&item.modelData.kind==="Joystick"&&!controller.sizeMoveMode){
     joystickIndex=i
     let px=root.usingGaze?root.gaze.x*root.screenWidth-root.logicalX-item.x:item.pointerX
     let py=root.usingGaze?root.gaze.y*root.screenHeight-root.logicalY-item.y:item.pointerY
     item.stickX=Math.max(-1,Math.min(1,(px-item.width/2)/(Math.min(item.width,item.height)*.4)))
     item.stickY=Math.max(-1,Math.min(1,(py-item.height/2)/(Math.min(item.width,item.height)*.4)))
     controller.joystick(i,item.stickX,item.stickY,true)
     target=""
    }
   }
   if(joystickIndex<0)controller.joystick(-1,0,0,false)
   if(root.overlapTargets.length>0){
    root.overlapProgress=controller.hoverKeys(root.overlapTargets)
    root.progress=0
   }else{
    root.overlapProgress=({})
    root.progress=controller.hover(target)
   }
  }
 }
 Rectangle {objectName:"keyboardBackdrop";anchors.fill:parent;visible:!root.transparentBackground;color:root.keyboardBackground}
 Repeater {
  id:keys
  onItemAdded: root.hitTestRevision++
  onItemRemoved: root.hitTestRevision++
  model:controller.layout.keys || []
  Rectangle {
   id:key
   onXChanged:Qt.callLater(root.updateInputMask)
   onYChanged:Qt.callLater(root.updateInputMask)
   onWidthChanged:Qt.callLater(root.updateInputMask)
   onHeightChanged:Qt.callLater(root.updateInputMask)
   clip:true
   required property var modelData
   required property int index
   property bool keyboardToggled:controller.keyToggled(modelData.commands||[])
   property bool gamepadPressed: {
    const commands=modelData.commands||[]
    return commands.length===1&&commands[0].tag==="Action"&&controller.gamepadHeld(commands[0].text.trim())
   }
   Connections {target:controller;function onChanged(){
    const commands=key.modelData.commands||[]
    key.keyboardToggled=controller.keyToggled(commands)
    key.gamepadPressed=commands.length===1&&commands[0].tag==="Action"&&controller.gamepadHeld(commands[0].text.trim())
   }}
   property real pointerX:0
   property real pointerY:0
   property real stickX:0
   property real stickY:0
   property string command:"key:"+index
   property string modifier:controller.modifierName(modelData.commands || [])
   property string modifierState:modifier?(controller.modifierStates[modifier] || ""):""
   property bool cycleButtonModeKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="CycleButtonMode"
   }
   property bool cycleKeyModeKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="CycleKeyMode"
   }
   property bool magnifierKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="MouseMagnifier"
   }
   property bool lookScrollKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="LookToScrollActive"
   }
   property bool stateFilled:((editAction==="MoveModeToggle"&&controller.canvasEditMode==="move")||(editAction==="SizeModeToggle"&&controller.canvasEditMode==="size"))||keyboardToggled||gamepadPressed||modifierState==="locked"||(magnifierKey&&controller.magnifierEnabled)||(sleepKey&&controller.paused)||(lookScrollKey&&controller.lookScrolling)
   property real predictionWidth:controller.layout.layoutMode==="canvas"?width/Math.max(1,predictionCount):2*root.width/controller.layout.cols
   property int predictionCount:modelData.suggestionCount!==undefined?modelData.suggestionCount:Math.floor(modelData.width/2)
   property bool sleepKey: {
    const commands=modelData.commands || []
    return commands.length===1&&commands[0].tag==="Action"&&commands[0].text.trim()==="Sleep"&&!unavailableReason
   }
   property bool resumeKey:controller.paused&&sleepKey
   enabled:!controller.paused||sleepKey
   property string unavailableReason:modelData.unavailableReason || ""
   property string label:root.minimizedRecovery?["↘","↙","↗","↖"][controller.restoreCorner]:controller.shifted&&modelData.shiftLabel?modelData.shiftLabel:modelData.label
   property real symbolMargin:controller.layout.symbolmargin!==undefined?Math.max(0,Number(controller.layout.symbolmargin)):12
   property color foreground:root.keyboardForeground
   property real foregroundOpacity:root.styleOpacity(modelData.style,"foregroundopacity")
   property string editAction:(modelData.commands||[]).length===1&&modelData.commands[0].tag==="Action"?modelData.commands[0].text.trim():""
   property bool sizeMoveKey:editAction==="MoveModeToggle"||editAction==="SizeModeToggle"
   property real editX:modelData.col
   property real editY:modelData.row
   property real editWidth:modelData.width
   property real editHeight:modelData.height
   x:(editMouse.pressed?editX:modelData.col)*root.width/controller.layout.cols
   y:(editMouse.pressed?editY:modelData.row)*root.height/controller.layout.rows
   width:(editMouse.pressed?editWidth:modelData.width)*root.width/controller.layout.cols
   height:(editMouse.pressed?editHeight:modelData.height)*root.height/controller.layout.rows
   color:root.keyboardPalette?root.keyboardPalette.key:Kirigami.Theme.backgroundColor
   // Composite overlapping key visuals before applying whole-key transparency.
   layer.enabled:opacity<1
   opacity:(enabled?1:0.35)*(root.minimizedRecovery?controller.restoreOpacity/100:root.styleOpacity(modelData.style,"opacity"))
   Kirigami.Theme.colorSet:Kirigami.Theme.Button
   Kirigami.Theme.inherit:false
   border.width:root.minimizedRecovery?0:1;border.color:root.keyboardBorder
   property bool active:enabled&&(root.overlapTargets.length>0?root.overlapTargets.indexOf(command)>=0:(root.usingGaze ? root.gazeTarget===command : root.hovered===command))
   Rectangle {anchors.fill:parent;color:root.dwellColor;opacity:key.active?0.18:0}
   Rectangle {
    anchors.fill:parent;anchors.margins:2
    visible:(key.sizeMoveKey&&controller.sizeMoveMode)||key.keyboardToggled||key.gamepadPressed||key.modifierState!==""||(key.magnifierKey&&controller.magnifierEnabled)||(key.sleepKey&&controller.paused)||(key.lookScrollKey&&controller.lookScrolling)
    color:key.stateFilled?root.dwellColor:"transparent"
    opacity:key.stateFilled?0.25:1
    border.width:controller.sizeMoveSelection===key.index?5:2;border.color:root.dwellColor
   }
   Text {
    anchors.fill:parent;anchors.margins:12;visible:key.modelData.kind==="Scratchpad"
    opacity:key.foregroundOpacity
    text:controller.scratchpad;color:key.foreground;wrapMode:Text.Wrap
    font.pixelSize:24;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter
   }
   Column {
    anchors.centerIn:parent;spacing:4
    opacity:key.foregroundOpacity
    visible:key.modelData.kind==="DynamicKey"&&!key.unavailableReason
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
   Rectangle {
    id:modeBadge
    opacity:key.foregroundOpacity
    objectName:"keyModeBadge:"+key.index
    visible:key.modelData.kind==="DynamicKey"&&!key.unavailableReason&&(key.cycleKeyModeKey||key.cycleButtonModeKey)&&!root.minimizedRecovery
    anchors.horizontalCenter:parent.horizontalCenter
    anchors.bottom:parent.bottom;anchors.bottomMargin:7
    width:Math.max(1,Math.min(key.width-12,modeBadgeText.implicitWidth+16))
    height:Math.max(16,Math.min(26,key.height*.22))
    radius:height/2
    color:key.color;border.width:1;border.color:key.foreground
    Text {
     id:modeBadgeText
     anchors.fill:parent;anchors.leftMargin:6;anchors.rightMargin:6
     text:key.cycleButtonModeKey?controller.buttonMode:controller.keyMode;color:key.foreground
     font.pixelSize:Math.max(10,Math.min(16,key.height*.16));font.bold:true
     minimumPixelSize:8;fontSizeMode:Text.Fit
     horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter
    }
   }
   Text {
    anchors.fill:parent;anchors.margins:6
    visible:key.unavailableReason.length>0
    opacity:key.foregroundOpacity
    text:"Unavailable\nin Plasma mode";color:key.foreground
    font.pixelSize:16;minimumPixelSize:10;fontSizeMode:Text.Fit
    horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter
    wrapMode:Text.WordWrap
   }
   Rectangle {
    visible:key.modelData.kind==="Joystick"
    anchors.centerIn:parent;width:Math.min(key.width,key.height)*.8;height:width;radius:width/2
    color:"transparent";border.color:key.foreground;opacity:.8*key.foregroundOpacity
    Rectangle {objectName:"joystickThumb:"+key.index;width:parent.width*.18;height:width;radius:width/2;color:root.dwellColor
     x:(parent.width-width)/2+(key.active?key.stickX:0)*parent.width*.35
     y:(parent.height-height)/2+(key.active?key.stickY:0)*parent.height*.35}
    Text {anchors.horizontalCenter:parent.horizontalCenter;anchors.bottom:parent.bottom;text:key.label||"Joystick";color:key.foreground;font.pixelSize:14}
   }
   MouseArea {
    objectName:"keyMouseArea:"+key.index
    anchors.fill:parent;hoverEnabled:true;enabled:true
    onEntered:root.hovered=(key.modelData.kind==="DynamicKey"||key.modelData.kind==="Joystick")?key.command:""
    onPositionChanged:mouse=>{key.pointerX=mouse.x;key.pointerY=mouse.y;if(key.modelData.kind==="Joystick")root.hovered=key.command}
    onExited:if(root.hovered===key.command)root.hovered=""
    onClicked:if(key.modelData.kind==="DynamicKey")controller.clickCommand(key.command)
   }
   Row {
    anchors.fill:parent;visible:key.modelData.kind==="SuggestionRow"
    Repeater {
     model:key.modelData.kind==="SuggestionRow"?key.predictionCount:0
     Rectangle {
      id:predictionSlot
      required property int index
      property string predictionText:index<controller.suggestions.length?controller.suggestions[index]:""
      property bool hasPrediction:predictionText.trim().length>0
      property string command:"suggest:"+index
      property bool active:key.enabled&&hasPrediction&&(root.usingGaze ? root.gazeTarget===command : root.hovered===command)
      onHasPredictionChanged:if(!hasPrediction&&root.hovered==="suggest:"+index)root.hovered=""
      width:key.predictionWidth;height:key.height;color:"transparent";border.color:root.keyboardBorder
      Rectangle {anchors.fill:parent;color:root.dwellColor;opacity:predictionSlot.active?0.18:0}
      Text {anchors.fill:parent;anchors.margins:4;text:predictionSlot.predictionText;opacity:key.foregroundOpacity;color:key.foreground;font.pixelSize:Math.min(24,key.height*.28);fontSizeMode:Text.Fit;horizontalAlignment:Text.AlignHCenter;verticalAlignment:Text.AlignVCenter}
      MouseArea {objectName:"predictionMouseArea:"+index;anchors.fill:parent;hoverEnabled:true;enabled:predictionSlot.hasPrediction;onEntered:root.hovered="suggest:"+index;onExited:if(root.hovered==="suggest:"+index)root.hovered="";onClicked:controller.clickCommand("suggest:"+index)}
      DwellIndicator {objectName:"predictionProgress:"+index;anchors.left:parent.left;anchors.bottom:parent.bottom;width:progress>0?parent.width:0;height:parent.height;progress:predictionSlot.active?root.progress:0;style:controller.dwellIndicatorStyle;color:root.dwellColor}
     }
    }
   }
   DwellIndicator {anchors.fill:parent;progress:key.active?(root.overlapTargets.length>0?(root.overlapProgress[key.command]||0):root.progress):0;style:controller.dwellIndicatorStyle;color:root.dwellColor}
   Rectangle {
    anchors.fill:parent;visible:controller.sizeMoveMode;color:"transparent"
    border.width:controller.sizeMoveSelection===key.index?5:2;border.color:root.dwellColor
    Text {anchors.right:parent.right;anchors.bottom:parent.bottom;visible:controller.canvasEditMode==="size";text:"◢";font.pixelSize:24;color:root.dwellColor}
   }
   MouseArea {
    id:editMouse
    objectName:"sizeMoveMouseArea:"+key.index
    anchors.fill:parent;enabled:controller.sizeMoveMode;hoverEnabled:true
    cursorShape:controller.canvasEditMode==="size"?Qt.SizeFDiagCursor:Qt.SizeAllCursor
    property point startPoint
    property real startX
    property real startY
    property real startWidth
    property real startHeight
    property bool resizing:false
    property bool moved:false
    onEntered:root.hovered=key.command
    onExited:root.hovered=""
    onPressed:mouse=>{
     startPoint=mapToItem(root.contentItem,mouse.x,mouse.y)
     startX=key.modelData.col;startY=key.modelData.row;startWidth=key.modelData.width;startHeight=key.modelData.height
     key.editX=startX;key.editY=startY;key.editWidth=startWidth;key.editHeight=startHeight
     resizing=controller.canvasEditMode==="size";moved=false;root.canvasDragging=true
     root.hovered=""
    }
    onPositionChanged:mouse=>{
     if(!pressed)return
     const point=mapToItem(root.contentItem,mouse.x,mouse.y)
     if(!moved&&Math.abs(point.x-startPoint.x)+Math.abs(point.y-startPoint.y)<4)return
     moved=true
     const dx=(point.x-startPoint.x)*controller.layout.cols/root.width
     const dy=(point.y-startPoint.y)*controller.layout.rows/root.height
     if(resizing){
      key.editWidth=Math.max(Math.min(startWidth,controller.layout.cols*24/root.width),Math.min(controller.layout.cols-startX,startWidth+dx))
      key.editHeight=Math.max(Math.min(startHeight,controller.layout.rows*24/root.height),Math.min(controller.layout.rows-startY,startHeight+dy))
     }else{
      key.editX=Math.max(0,Math.min(controller.layout.cols-startWidth,startX+dx))
      key.editY=Math.max(0,Math.min(controller.layout.rows-startHeight,startY+dy))
     }
    }
    onCanceled:root.canvasDragging=false
    onReleased:{
     root.canvasDragging=false
     if(moved)controller.setCanvasKeyRect(key.index,key.editX,key.editY,key.editWidth,key.editHeight)
     else controller.clickCommand(key.command)
    }
   }
   Accessible.role:Accessible.Button
   Accessible.description:key.keyboardToggled?"Locked until selected again":key.cycleButtonModeKey?"Button mode: "+controller.buttonMode:key.cycleKeyModeKey?"Key mode: "+controller.keyMode:key.sleepKey?(controller.paused?"Pause on":"Pause off"):key.magnifierKey?(controller.magnifierEnabled?"Magnifier on":"Magnifier off"):key.modifier?(key.modifierState==="locked"?"Locked until selected again":key.modifierState==="latched"?"Latched for the next key":"Off. Select once to latch, twice to lock."):""
   Accessible.name:key.sleepKey?(controller.paused?"Resume input":"Pause input"):root.minimizedRecovery?"Restore keyboard":key.unavailableReason?key.unavailableReason:(key.label||key.modelData.symbol)
   Accessible.onPressAction:controller.clickCommand(key.command)
  }
 }
 Rectangle {
  z:45;visible:controller.sizeMoveMode&&controller.canvasEditPreview.col!==undefined
  x:(controller.canvasEditPreview.col||0)*root.width/controller.layout.cols
  y:(controller.canvasEditPreview.row||0)*root.height/controller.layout.rows
  width:(controller.canvasEditPreview.width||0)*root.width/controller.layout.cols
  height:(controller.canvasEditPreview.height||0)*root.height/controller.layout.rows
  color:"transparent";border.width:3;border.color:root.dwellColor
 }
 DwellIndicator {
  z:46;visible:controller.sizeMoveMode
  x:root.canvasPoint.x-24;y:root.canvasPoint.y-24;width:48;height:48
  progress:root.progress;style:"ring";color:root.dwellColor
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
  id:settings;objectName:"settingsWindow";visible:false;width:Math.min(1000,root.screenWidth);height:Math.min(720,root.screenHeight-60);title:"GazeGrid settings";color:Kirigami.Theme.backgroundColor
  minimumWidth:Math.min(760,root.screenWidth);minimumHeight:Math.min(500,root.screenHeight-60)
  property var categoryNames:["Status","Keyboard","Gaze & dwell","Magnifier","Typing & speech","Desktop access","Shortcuts","Look to scroll"]
  property var categoryIcons:["dialog-information","input-keyboard","input-mouse","zoom-in","edit-text-frame-update","preferences-desktop","preferences-desktop-keyboard-shortcuts","transform-move"]
  component SettingSlider:ColumnLayout {
   property string label
   property string displayValue
   property alias from:control.from
   property alias to:control.to
   property alias stepSize:control.stepSize
   property alias value:control.value
   signal moved()
   Layout.fillWidth:true;spacing:2
   RowLayout {
    Layout.fillWidth:true
    Desktop.Label {text:label;Layout.fillWidth:true;wrapMode:Text.Wrap}
    Desktop.Label {text:displayValue;font.bold:true;horizontalAlignment:Text.AlignRight}
   }
   Desktop.Slider {
    id:control;Layout.fillWidth:true;Accessible.name:label
    onMoved:parent.moved()
   }
  }
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
    Layout.preferredWidth:210;Layout.fillHeight:true
    color:Kirigami.Theme.backgroundColor
    Kirigami.Theme.colorSet:Kirigami.Theme.View
    Kirigami.Theme.inherit:false
    ColumnLayout {
     anchors.fill:parent;anchors.margins:Kirigami.Units.largeSpacing;spacing:Kirigami.Units.largeSpacing
     Kirigami.Heading {text:"Settings";level:1;Layout.topMargin:8;Layout.leftMargin:8}
     ListView {
     id:settingsCategory;objectName:"settingsCategory"
     Layout.fillWidth:true;Layout.fillHeight:true;clip:true;spacing:4
     model:settings.categoryNames;currentIndex:1
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
   }
   Kirigami.Separator {Layout.fillHeight:true}
   ColumnLayout {
    Layout.fillWidth:true;Layout.fillHeight:true;Layout.margins:Kirigami.Units.largeSpacing*2;spacing:Kirigami.Units.largeSpacing
    RowLayout {
     Layout.fillWidth:true;Layout.bottomMargin:4
     Kirigami.Heading {text:settings.categoryNames[settingsCategory.currentIndex];level:1;Layout.fillWidth:true}
     Desktop.Label {text:controller.paused?"Input paused":"Input active"}
    }
   Desktop.ScrollView {
    id:settingsScroll;Layout.fillWidth:true;Layout.fillHeight:true;contentWidth:availableWidth;clip:true;ScrollBar.horizontal.policy:ScrollBar.AlwaysOff
    ColumnLayout {
     width:settingsScroll.availableWidth;spacing:12
     Desktop.GroupBox {
      title:settings.categoryNames[0];visible:settingsCategory.currentIndex==0;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.Label {text:!root.gazeEnabled?"Gaze off":root.gazeValid?"Gaze: receiving":"Gaze unavailable — mouse dwell active";Layout.fillWidth:true;wrapMode:Text.Wrap}
       Desktop.Label {text:"Desktop control: "+controller.portalStatus;Layout.fillWidth:true;wrapMode:Text.Wrap}
       Desktop.Label {text:"Last action: "+controller.actionStatus;Layout.fillWidth:true;wrapMode:Text.Wrap}
       Desktop.Label {text:"Eye tracker: "+controller.tobiiStatus;Layout.fillWidth:true;wrapMode:Text.Wrap}
       Text {visible:text.length>0;text:controller.magneticCursor?"Cursor follows gaze":"";color:Kirigami.Theme.positiveTextColor}
       Text {visible:text.length>0;text:controller.lookScrolling?"Look to scroll active":"";color:Kirigami.Theme.positiveTextColor}
       Text {visible:text.length>0;text:controller.capturePending?"Waiting for desktop capture":controller.magnifierEnabled?"Magnifier on":"";color:Kirigami.Theme.positiveTextColor}
       Text {visible:text.length>0;text:controller.heldKeys.length?"Held: "+controller.heldKeys.join(", "):"";color:Kirigami.Theme.neutralTextColor}
       Text {visible:text.length>0;text:controller.heldPointerButtons.length?"Mouse held: "+controller.heldPointerButtons.join(", "):"";color:Kirigami.Theme.neutralTextColor}
       Tool {id:recover;text:"Home / recover";command:"Recover"}
       Desktop.Button {text:"Quit GazeGrid";icon.name:"application-exit";onClicked:Qt.quit()}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[1];visible:settingsCategory.currentIndex==1;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
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
       Desktop.CheckBox {
        visible:controller.layout.layoutMode==="canvas"
        text:"Save move/resize changes to original XML"
        checked:controller.saveCanvasChanges
        onToggled:controller.saveCanvasChanges=checked
       }
       Desktop.Button {
        visible:controller.layout.layoutMode==="canvas"
        text:"Save current positions and sizes to XML"
        onClicked:controller.saveCanvasGeometry()
       }
       Desktop.Label {
        visible:controller.layout.layoutMode==="canvas";Layout.fillWidth:true;wrapMode:Text.WordWrap
        text:controller.canvasSaveStatus||"Saving overwrites this keyboard's original XML. Automatic saving applies to completed moves and resizes."
       }
      }
     }
     Desktop.GroupBox {
      title:"Appearance";      visible:settingsCategory.currentIndex==1;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.Label {text:"Keyboard theme"}
       Desktop.ComboBox {
        objectName:"keyboardThemeSelector";Layout.fillWidth:true
        model:root.themeNames
        currentIndex:Math.max(0,root.themeIds.indexOf(controller.keyboardTheme))
        onActivated:controller.keyboardTheme=root.themeIds[currentIndex]
        Accessible.name:"Keyboard theme"
       }
       Desktop.Label {text:"Dwell progress style"}
       Desktop.ComboBox {
        objectName:"dwellStyleSelector";Layout.fillWidth:true;model:root.dwellStyleNames
        currentIndex:Math.max(0,root.dwellStyleIds.indexOf(controller.dwellIndicatorStyle))
        onActivated:controller.dwellIndicatorStyle=root.dwellStyleIds[currentIndex]
        Accessible.name:"Dwell progress style"
       }
       Rectangle {
        Layout.fillWidth:true;Layout.preferredHeight:72;color:root.keyboardBackground;border.color:root.keyboardBorder;radius:4
        Desktop.Label {anchors.centerIn:parent;text:"Dwell preview"}
        DwellIndicator {
         id:dwellPreview;anchors.fill:parent;style:controller.dwellIndicatorStyle;color:root.dwellColor
         NumberAnimation on progress {from:0;to:1;duration:controller.dwellMs;loops:Animation.Infinite;running:settings.visible&&settingsCategory.currentIndex===1}
        }
       }
       SettingSlider {label:"Keyboard height";displayValue:controller.keyboardScale+"%";from:50;to:150;stepSize:5;value:controller.keyboardScale;onMoved:controller.keyboardScale=value}
       SettingSlider {label:"Short key labels";displayValue:controller.shortLabelScale+"%";from:50;to:200;stepSize:5;value:controller.shortLabelScale;onMoved:controller.shortLabelScale=value}
       SettingSlider {label:"Long key labels";displayValue:controller.longLabelScale+"%";from:50;to:200;stepSize:5;value:controller.longLabelScale;onMoved:controller.longLabelScale=value}
       Desktop.CheckBox {text:"Temporarily hide keyboard during mouse actions";checked:controller.hideKeyboardDuringMouseActions;onToggled:controller.hideKeyboardDuringMouseActions=checked}
      }
     }
     Desktop.GroupBox {
      title:"Restore button";      visible:settingsCategory.currentIndex==1;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       RowLayout {
        Desktop.Label {text:"Restore button corner"}
        Desktop.ComboBox {model:["Top left","Top right","Bottom left","Bottom right"];currentIndex:controller.restoreCorner;onActivated:controller.restoreCorner=currentIndex}
       }
       SettingSlider {label:"Restore button width";displayValue:controller.restoreWidth+" px";from:40;to:128;stepSize:8;value:controller.restoreWidth;onMoved:controller.restoreWidth=value}
       SettingSlider {label:"Restore button height";displayValue:controller.restoreHeight+" px";from:40;to:128;stepSize:8;value:controller.restoreHeight;onMoved:controller.restoreHeight=value}
       SettingSlider {label:"Restore button opacity";displayValue:controller.restoreOpacity+"%";from:10;to:100;stepSize:5;value:controller.restoreOpacity;onMoved:controller.restoreOpacity=value}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[2];visible:settingsCategory.currentIndex==2;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.CheckBox {text:"Enable gaze input";checked:root.gazeEnabled;enabled:!root.gazeCheck;focusPolicy:Qt.NoFocus;onToggled:controller.setGazeEnabled(checked)}
       Tool {id:tobii;text:"Connect eye tracker";command:"Tobii"}
      }
     }
     Desktop.GroupBox {
      title:"Gaze smoothing";      visible:settingsCategory.currentIndex==2;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       RowLayout {
        Desktop.Label {text:"Gaze smoothing"}
        Desktop.ComboBox {model:["0 — Off","1 — Low","2 — Medium","3 — High","4 — Extreme"];currentIndex:controller.gazeSmoothingLevel;onActivated:controller.gazeSmoothingLevel=currentIndex}
       }
       Desktop.CheckBox {text:"Smooth when changing gaze target";checked:controller.smoothWhenChangingGazeTarget;enabled:controller.gazeSmoothingLevel>0;onToggled:controller.smoothWhenChangingGazeTarget=checked}
      }
     }
     Desktop.GroupBox {
      title:"Gaze indicator";      visible:settingsCategory.currentIndex==2;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       SettingSlider {label:"Size";displayValue:controller.gazeIndicatorSize+" px";from:4;to:120;stepSize:1;value:controller.gazeIndicatorSize;onMoved:controller.gazeIndicatorSize=value}
       SettingSlider {label:"Opacity";displayValue:controller.gazeIndicatorOpacity+"%";from:5;to:100;stepSize:5;value:controller.gazeIndicatorOpacity;onMoved:controller.gazeIndicatorOpacity=value}
       RowLayout {
        Desktop.Label {text:"Color"}
        Rectangle {width:24;height:24;radius:12;color:controller.gazeIndicatorColor;opacity:controller.gazeIndicatorOpacity/100}
        Desktop.TextField {text:controller.gazeIndicatorColor;placeholderText:"#RRGGBB";Accessible.name:"Gaze indicator color, hexadecimal RGB";validator:RegularExpressionValidator {regularExpression:/#[0-9a-fA-F]{6}/} onEditingFinished:{if(acceptableInput)controller.gazeIndicatorColor=text}}
       }
       Desktop.CheckBox {text:"Morph with gaze acceleration and direction";checked:controller.gazeIndicatorMorph;onToggled:controller.gazeIndicatorMorph=checked}
      }
     }
     Desktop.GroupBox {
      title:"Keyboard selection";      visible:settingsCategory.currentIndex==2;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Tool {text:"Button mode: "+controller.buttonMode+" (cycle)";command:"CycleButtonMode"}
       Tool {text:"Key mode: "+controller.keyMode+" (cycle)";command:"CycleKeyMode"}
       Desktop.Label {text:"Tap → Repeat → Hold → Toggle. Hold presses after dwell and releases when mouse or gaze leaves. Toggle keeps a key pressed until selected again. Applies to all physical keyboard keys except modifiers (Shift, Ctrl, Alt and Super/Windows).";wrapMode:Text.Wrap;Layout.fillWidth:true}
       Desktop.CheckBox {text:"Preserve toggled keys between layouts";checked:controller.preserveTogglesBetweenLayouts;onToggled:controller.preserveTogglesBetweenLayouts=checked}
       Desktop.CheckBox {text:"Cycle modifiers while hovering";checked:controller.cycleModifiersOnHover;onToggled:controller.cycleModifiersOnHover=checked}
       SettingSlider {label:"Repeat rate";displayValue:(1000/controller.repeatIntervalMs).toFixed(1)+" / sec ("+controller.repeatIntervalMs+" ms)";from:100;to:2000;stepSize:50;value:controller.repeatIntervalMs;onMoved:controller.repeatIntervalMs=value;Accessible.name:"Repeat interval in milliseconds"}
       SettingSlider {label:"Keyboard dwell";displayValue:controller.dwellMs+" ms";from:250;to:3000;stepSize:50;value:controller.dwellMs;onMoved:controller.dwellMs=value}

      }
     }
     Desktop.GroupBox {
      title:"Zoom behavior";visible:settingsCategory.currentIndex==3;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       SettingSlider {label:"Magnifier zoom";displayValue:controller.magnifierZoom+"×";from:2;to:8;stepSize:1;value:controller.magnifierZoom;onMoved:controller.magnifierZoom=value}
       Desktop.CheckBox {text:"Progressive zoom";checked:controller.progressiveZoom;onToggled:controller.progressiveZoom=checked}
       SettingSlider {label:"Zoom duration";displayValue:(controller.zoomDurationMs/1000).toFixed(1)+" s";from:500;to:5000;stepSize:100;value:controller.zoomDurationMs;enabled:controller.progressiveZoom;onMoved:controller.zoomDurationMs=value}
      }
     }
     Desktop.GroupBox {
      title:"Target selection";      visible:settingsCategory.currentIndex==3;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.CheckBox {text:"Select pointer targets by dwell";checked:controller.targetDwellEnabled;onToggled:controller.targetDwellEnabled=checked}
       SettingSlider {label:"Start zoom / target dwell";displayValue:controller.targetStartDwellMs+" ms";from:250;to:3000;stepSize:50;value:controller.targetStartDwellMs;enabled:controller.targetDwellEnabled;onMoved:controller.targetStartDwellMs=value}
       SettingSlider {label:"Confirm zoomed target dwell";displayValue:controller.targetConfirmDwellMs+" ms";from:250;to:3000;stepSize:50;value:controller.targetConfirmDwellMs;enabled:controller.targetDwellEnabled;onMoved:controller.targetConfirmDwellMs=value}
       SettingSlider {label:"Target dwell movement tolerance";displayValue:controller.targetDwellRadius+" px";from:10;to:100;stepSize:5;value:controller.targetDwellRadius;enabled:controller.targetDwellEnabled;onMoved:controller.targetDwellRadius=value}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[4];visible:settingsCategory.currentIndex==4;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.CheckBox {visible:Qt.platform.os!=="windows";text:"Literal keys (bypass platform mappings)";checked:controller.literal;onToggled:controller.literal=checked}
      }
     }
     Desktop.GroupBox {
      title:"Word prediction";      visible:settingsCategory.currentIndex==4;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.Label {text:"Prediction style"}
       Desktop.ComboBox {
        objectName:"predictionModeSelector";Layout.fillWidth:true
        model:["Context-aware","Common words","Off"]
        currentIndex:["context","frequency","off"].indexOf(controller.predictionMode)
        onActivated:controller.predictionMode=["context","frequency","off"][currentIndex]
        Accessible.name:"Prediction style"
       }
       Desktop.CheckBox {objectName:"suggestSentenceStartersToggle";text:"Suggest sentence starters when text is empty";enabled:controller.predictionMode!=="off";checked:controller.suggestSentenceStarters;onToggled:controller.suggestSentenceStarters=checked}
       Desktop.CheckBox {objectName:"addSpaceAfterCompletionToggle";text:"Add a space after word completion";enabled:controller.predictionMode!=="off";checked:controller.addSpaceAfterCompletion;onToggled:controller.addSpaceAfterCompletion=checked}
       Desktop.CheckBox {objectName:"predictNextWordToggle";text:"Suggest the next word after a space";enabled:controller.predictionMode!=="off";checked:controller.predictNextWord;onToggled:controller.predictNextWord=checked}
       Desktop.CheckBox {text:"Use and learn personal words locally";checked:controller.learning;onToggled:controller.learning=checked}
       Desktop.Button {text:"Reset learned words";onClicked:controller.resetLearning()}
      }
     }
     Desktop.GroupBox {
      title:"Speech";      visible:settingsCategory.currentIndex==4;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Flow {Layout.fillWidth:true;spacing:8
        Tool {id:speakButton;text:"Speak";command:"Speak"}
        Tool {id:stopSpeechButton;text:"Stop speech";command:"StopSpeech"}
       }
       Desktop.Label {text:"Scratchpad (text typed by GazeGrid only)"}
       Desktop.TextArea {text:controller.scratchpad;readOnly:true;Layout.fillWidth:true;Layout.preferredHeight:70;wrapMode:Text.Wrap}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[5];visible:settingsCategory.currentIndex==5;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Tool {id:authorize;text:"Connect desktop control";command:"Connect"}
       Desktop.CheckBox {visible:Qt.platform.os!=="windows";text:"Remember desktop-control permission, if supported";checked:controller.rememberDesktop;onToggled:controller.rememberDesktop=checked}
       Desktop.Button {visible:Qt.platform.os!=="windows";text:"Disconnect and forget desktop permission";onClicked:controller.forgetDesktopPermission()}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[6];visible:settingsCategory.currentIndex==6;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       Desktop.Label {text:Qt.platform.os==="windows"?"Windows uses the keyboard pack’s original shortcuts.":"Linux shortcut mappings"}
       Desktop.ScrollView {visible:Qt.platform.os!=="windows";Layout.fillWidth:true;Layout.preferredHeight:180;Desktop.TextArea {id:mappingEditor;text:controller.mappingJson;wrapMode:Text.Wrap;font.family:"monospace";selectByMouse:true}}
       Desktop.Button {visible:Qt.platform.os!=="windows";text:"Save mappings";onClicked:controller.saveMappings(mappingEditor.text)}
       Desktop.Label {text:controller.actionStatus;wrapMode:Text.Wrap;Layout.fillWidth:true}
      }
     }
     Desktop.GroupBox {
      title:settings.categoryNames[7];visible:settingsCategory.currentIndex==7;Layout.fillWidth:true
      ColumnLayout {
       anchors.fill:parent;spacing:Kirigami.Units.largeSpacing
       SettingSlider {label:"Scroll speed";displayValue:controller.scrollSpeed+"%";from:10;to:300;stepSize:10;value:controller.scrollSpeed;onMoved:controller.scrollSpeed=value}
       SettingSlider {label:"Centre dead zone";displayValue:controller.scrollDeadzone+" px";from:10;to:200;stepSize:5;value:controller.scrollDeadzone;onMoved:controller.scrollDeadzone=value}
       RowLayout {Desktop.Label {text:"Direction"} Desktop.ComboBox {model:["Both axes","Vertical only","Horizontal only"];currentIndex:controller.scrollAxis;onActivated:controller.scrollAxis=currentIndex}}
      }
     }
    }
   }
   Desktop.Label {text:controller.profileStatus;visible:text.length>0;wrapMode:Text.Wrap;Layout.fillWidth:true}
   Kirigami.Separator {Layout.fillWidth:true}
   RowLayout {
    Layout.fillWidth:true;spacing:12
    Desktop.Button {text:"Reset preferences";onClicked:controller.resetPreferences()}
    Item {Layout.fillWidth:true}
    Desktop.Button {text:"Close";onClicked:settings.close()}
    Desktop.Button {text:"Save preferences";icon.name:"document-save";highlighted:true;onClicked:controller.savePreferences()}
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
   DwellIndicator {anchors.fill:parent;progress:root.hovered==="CancelTarget"?root.progress:0;style:controller.dwellIndicatorStyle;color:Kirigami.Theme.highlightColor}
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
