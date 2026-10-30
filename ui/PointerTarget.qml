import QtQuick
Window {
 visible:false;title:"gaze-grid disposable pointer test";color:"#e9f1f7"
 Text {
  anchors.horizontalCenter:parent.horizontalCenter;y:parent.height*.1
  text:"gaze-grid pointer verification — keep this receiver focused"
  font.pixelSize:26;color:"#152739"
 }
 Rectangle {x:parent.width*.2;y:parent.height*.18;width:parent.width*.6;height:parent.height*.28;color:"#c2ded9";radius:12}
 MouseArea {anchors.fill:parent;acceptedButtons:Qt.AllButtons;hoverEnabled:true;onWheel:wheel=>wheel.accepted=true}
}
