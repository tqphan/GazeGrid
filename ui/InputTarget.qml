import QtQuick
import QtQuick.Controls
Window {
 width: 700; height: 260; visible: false; title: "OptiKey disposable input test"; color: "#f3f6fa"
 Column { anchors.fill: parent; anchors.margins: 24; spacing: 16
  Text { text: "OptiKey input verification"; font.pixelSize: 24 }
  Text { text: "Click this window if needed. It closes after receiving the test input." }
  TextArea { width: parent.width; height: 100; focus: true; placeholderText: "Waiting for optikey + Enter…" }
 }
}
