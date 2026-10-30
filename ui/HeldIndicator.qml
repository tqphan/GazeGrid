import QtQuick

Item {
 id: indicator
 property string style: ""
 property color color: "#00bfff"
 property real phase: 0
 visible: style !== ""
 clip: true
 Canvas {
  id: outline
  anchors.fill: parent
  visible: indicator.style === "animated-outline"
  onWidthChanged: requestPaint()
  onHeightChanged: requestPaint()
  onVisibleChanged: requestPaint()
  Connections {
   target: indicator
   function onPhaseChanged() { if (outline.visible) outline.requestPaint() }
   function onColorChanged() { outline.requestPaint() }
  }
  onPaint: {
   const ctx=getContext("2d")
   ctx.reset()
   const w=width-4,h=height-4,p=2*(w+h)
   if(w<=0 || h<=0) return
   function point(distance) {
    const d=distance%p
    if(d<=w) return [2+d,2]
    if(d<=w+h) return [2+w,2+d-w]
    if(d<=2*w+h) return [2+2*w+h-d,2+h]
    return [2,2+p-d]
   }
   const start=indicator.phase*p,end=start+p*0.3
   let pos=point(start)
   ctx.strokeStyle=indicator.color;ctx.lineWidth=3
   ctx.beginPath();ctx.moveTo(pos[0],pos[1])
   const corners=[w,w+h,2*w+h,p]
   for(let lap=0;lap<2;lap++) {
    for(let i=0;i<corners.length;i++) {
     const distance=lap*p+corners[i]
     if(distance>start && distance<end) {
      pos=point(distance);ctx.lineTo(pos[0],pos[1])
     }
    }
   }
   pos=point(end);ctx.lineTo(pos[0],pos[1]);ctx.stroke()
  }
 }
 NumberAnimation on phase {
  from: 0; to: 1; duration: indicator.style === "sweep" ? 1800 : 2200
  loops: Animation.Infinite
  running: indicator.visible && ["pulse","ripple","sweep","animated-outline"].indexOf(indicator.style) >= 0
 }
 Rectangle {
  anchors.fill: parent
  visible: indicator.style !== "underline"
  color: ["fill","pulse"].indexOf(indicator.style) >= 0 ? indicator.color : "transparent"
  border.color: indicator.color; border.width: 2
  opacity: indicator.style === "fill" ? 0.25 : indicator.style === "pulse" ? 0.2+0.35*(1-Math.cos(indicator.phase*2*Math.PI))/2 : indicator.style === "ripple" || indicator.style === "sweep" || indicator.style === "animated-outline" ? 0.4 : 1
 }
 Rectangle {
  visible: indicator.style === "ripple"
  anchors.centerIn: parent
  width: parent.width*indicator.phase; height: parent.height*indicator.phase
  color: "transparent"; border.color: indicator.color
  border.width: Math.min(2,width/2,height/2)
  opacity: 1-indicator.phase
 }
 Rectangle {
  visible: indicator.style === "underline" || indicator.style === "sweep"
  width: indicator.style === "sweep" ? parent.width*0.25 : parent.width
  height: Math.min(4,parent.height)
  y: parent.height-height
  x: indicator.style === "sweep" ? (parent.width-width)*(1-Math.cos(indicator.phase*2*Math.PI))/2 : 0
  color: indicator.color
 }
}
