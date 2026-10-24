import QtQuick
import QtQuick.Controls
Window {
 width: 640; height: 300; visible: true; title: "GazeGrid native input-method test"
 property string receivedText: normal.text
 property bool privateComplete: password.text === "secret42"
 property bool wasActive: false
 property bool focusLost: false
 onActiveChanged: {
  if (active) wasActive = true
  else if (wasActive) focusLost = true
 }
 Column {
  anchors.fill: parent; anchors.margins: 20; spacing: 12
  Label { text: "Isolated native Unicode, editing and private-field check" }
  TextArea {
   id: normal; width: parent.width; height: 100; focus: true
   onTextChanged: if(text === "GazeGrid é€😀\n") password.forceActiveFocus()
  }
  TextField { id: password; width: parent.width; echoMode: TextInput.Password; inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData }
 }
 Component.onCompleted: {requestActivate();normal.forceActiveFocus()}
}
