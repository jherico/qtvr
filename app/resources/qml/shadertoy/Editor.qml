import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../windows"
import "../controls"

Window {
    width: 800
    height: 800

    Item {
        anchors.fill: parent;

        FontLoader { id: hackFont; source: "../../fonts/Hack-Regular.ttf"; }

        Rectangle {
            anchors { left: parent.left; right: textureColumn.left; rightMargin: 8; top: parent.top; bottom: parent.bottom; }
            color: "#222"

            ScrollView {
                anchors.fill: parent;
                id: flick
                clip: true

                function ensureVisible(r) {
                    if (flickableItem.contentY >= r.y) {
                        flickableItem.contentY = r.y;
                    } else if (flickableItem.contentY+height <= r.y+r.height) {
                        flickableItem.contentY = r.y+r.height-height;
                    }
                }

                TextEdit {
                    id: textEdit
                    height: implicitHeight
                    width: implicitWidth
                    objectName: "glslEditor"
                    font.family: hackFont.name
                    wrapMode: TextEdit.NoWrap
                    text: "// 25 boxes, a tunnel based on voronoi, bit encoded patterns, script for 80 seconds (music)\n// fragment shader by movAX13h, November 2013\n\n// NOTE: Patterns start at ~45 seconds.\n#pragma vr\n\nfloat rand(vec2 co) {\n  return fract(sin(dot(co.xy, vec2(12.9898, 78.233))) * 43758.5453);\n}\n\nfloat rand(float n) {\n  return fract(sin(n * 12.9898) * 43758.5453);\n}\n\nvec2 rand2(vec2 p) {\n  float r = 523.0 * sin(dot(p, vec2(53.3158, 43.6143)));\n  return vec2(fract(15.32354 * r), fract(17.25865 * r));\n}\n\nfloat sdBox(vec3 p, vec3 b) {\n  vec3 d = abs(p) - b;\n  return min(max(d.x, max(d.y, d.z)), 0.0) + length(max(d, 0.0));\n}\n\nfloat pattern(float n, vec2 p) {\n  p = p * 4.0;\n  p = floor(p + 2.5);\n\n  if (clamp(p.x, 0.0, 4.0) == p.x && clamp(p.y, 0.0, 4.0) == p.y) {\n    float k = p.x + p.y * 5.0;\n    if (int(mod(n / (pow(2.0, k)), 2.0)) == 1)\n      return 0.0;\n  }\n\n  return 1.0;\n}\n\nstruct Cell {\n  float d;\n  vec2 hash;\n  vec2 pos;\n};\n\nCell Cells(in vec2 p, in float numCells, bool bump)\n{\n  p *= numCells;\n\n  float d = 1.0e20;\n  vec2 hash;\n  vec2 pos;\n\n  for (int dx = -1; dx <= 1; dx++)\n  for (int dy = -1; dy <= 1; dy++)\n  {\n    vec2 tp = floor(p) + vec2(dx, dy);\n    vec2 h = rand2(vec2(mod(tp.x, numCells), tp.y)); // repeat x\n    float m = length(p - tp - h);\n\n    if (m < d)\n    {\n      d = m;\n      hash = h;\n      pos = tp;\n    }\n  }\n\n  if (bump) return Cell(d, hash, pos);\n  return Cell(1.0-d, hash, pos);\n}\n\nfloat sampleMusic(float f, float bands) {\n  f = floor(f * bands) / bands;\n  float fft = texture2D(iChannel0, vec2(f, 0.0)).x;\n  return fft;\n}\n\nfloat sampleMusic() {\n  return 0.25\n      * (texture2D(iChannel0, vec2(0.01, 0.25)).x\n          + texture2D(iChannel0, vec2(0.07, 0.25)).x\n          + texture2D(iChannel0, vec2(0.15, 0.25)).x\n          + texture2D(iChannel0, vec2(0.30, 0.25)).x);\n}\n\n#define L_hi 1613493.0\n#define L_smile 10813998.0\n#define L_I 14815374.0\n#define L_heart 11533764.0\n#define L_S 15793695.0\n#define L_T 462976.0\n\n#define L_NONE 0.0\n#define L_RANDOM 1.0\n\nfloat time;\n\nbool openTunnel = false;\nbool bumpTunnel = false;\nfloat boxSymbol = L_RANDOM;\n\nvec4 scene(vec3 p) {\n  float tunnelRadius = 0.38;\n  float numCells = 8.0;\n\n  float d, d1;\n  vec3 q = p;\n\n  vec3 col = vec3(0.1, 0.7, 1.0);\n  d = 10000.0;\n\n  // tunnel\n#if 1\n  vec2 uv = vec2((atan(p.y, p.x) + 3.14159265) / 6.283185307,\n      -time * 0.6 + p.z * 0.4);\n  Cell cell = Cells(uv, numCells, bumpTunnel);\n  q.xy *= 1.0 + cell.d * 0.1;\n  d = max(length(q.xy) - tunnelRadius + 0.01, -(length(q.xy) - tunnelRadius));\n  if (openTunnel)\n    d = max(-sdBox(p - vec3(-1.2, 0.0, 0.0), vec3(1.0, 1.0, 10.0)), d);\n  float m = sampleMusic()\n      * (1.0 + smoothstep(0.4, 0.6, sampleMusic(cell.hash.x * cell.hash.y, 4.0)));\n  vec3 c = m * vec3(cell.hash.x * 0.9, cell.hash.y * 0.3, 0.11) * cell.d;\n  col = mix(col, c, smoothstep(0.1, 0.0, d));\n#endif\n\n  // letter boxes\n#if 1\n  float ltime = mod(time, 10.0);\n\n  if (boxSymbol > 0.0) {\n    for (int i = 0; i < 25; i++) {\n      vec3 pos;\n\n      if (boxSymbol < 1.1) // random boxes\n          {\n        ltime = time * 3.0 + float(i) * 20.134;\n\n        float r = rand(float(i) * 20.33);\n        float z = -6.0 + mod(ltime * (r + 0.5), 15.0);\n\n        if (!openTunnel && z > 2.0)\n          continue;\n\n        pos = vec3(0.04 * mod(float(i), 5.0) - 0.08,\n            0.04 * floor(float(i) / 5.0) - 0.08, z);\n        d1 = sdBox(p - pos, vec3(0.009)); // ugly boxes\n        if (d1 < d) {\n          d = d1;\n          col = vec3(0.1, 0.6, 0.9) * (r + 0.2);\n        }\n      } else // symbol mode\n      {\n        float z = 0.5 + 0.2 * tan(-time - float(i) * 0.04);\n        if (z > 1.0)\n          continue;\n\n        vec3 shift = min(z - 0.5, 0.0)\n            * vec3(0.08 * sin(time + 0.2 * float(i)),\n                0.08 * cos(time + 0.4 * float(i)), 0.0);\n        pos = vec3(0.04 * mod(float(i), 5.0) - 0.08,\n            0.04 * floor(float(i) / 5.0) - 0.08, z);\n        d1 = sdBox(p - pos - shift, vec3(0.009)); // ugly boxes\n        if (d1 < d) {\n          float lv = pattern(boxSymbol, (p.xy - shift.xy) * 6.2);\n          d = d1;\n          if (lv > 0.5)\n            col = vec3(0.216, 0.106, 0.173);\n          else\n            col = vec3(0.820, 0.839, 0.906);\n        }\n      }\n    }\n  }\n#endif\n  return vec4(col, d);\n}\n\nvoid main(void) {\n  vec2 pos = (gl_FragCoord.xy * 2.0 - iResolution.xy) / iResolution.y;\n\n  float focus = 3.14;\n  float far = 5.5;\n\n  time = iGlobalTime; // iChannelTime[0]; \n\n  vec3 ct = vec3(0.0);\n  vec3 cp;\n  vec3 cu = vec3(0.0, 1.0, 0.0);\n\n  if (time < 12.7) {\n    cp = vec3(0.2 * sin(time * 0.4), -0.02 + 0.2 * cos(time * 0.2), 1.5);\n  } else if (time < 27.8) {\n    cp = vec3(-0.2 * sin(time * 0.4), 0.02 + 0.2 * sin(time * 0.2), 1.5);\n  } else if (time < 35.4) {\n    bumpTunnel = true;\n    boxSymbol = L_NONE;\n    cp = vec3(0.6, sin(time * 0.4 - 35.4), 1.5);\n  } else if (time < 44.9) {\n    openTunnel = true;\n    cp = vec3(-2.0, 0.0, 3.5);\n  } else if (time < 45.2) {\n    boxSymbol = L_NONE;\n    cp = vec3(0.0, 0.0, 1.5);\n  } else if (time < 63.7) {\n    cp = vec3(0.0, 0.0, 1.5);\n    cu = vec3(0.1 * sin(time), 1.0, 0.1 * cos(time));\n\n    float id = mod(floor((time - 45.2) / 3.1415), 6.0);\n    if (id == 0.0)\n      boxSymbol = L_hi;\n    else if (id == 1.0)\n      boxSymbol = L_smile;\n    else if (id == 2.0)\n      boxSymbol = L_I;\n    else if (id == 3.0)\n      boxSymbol = L_heart;\n    else if (id == 4.0)\n      boxSymbol = L_S;\n    else if (id == 5.0)\n      boxSymbol = L_T;\n  } else if (time < 79.3) {\n    boxSymbol = L_NONE;\n    cu = vec3(sin(time), 1.0, cos(time));\n    cp = vec3(0.0, 0.0, 1.5);\n  } else {\n    boxSymbol = L_RANDOM;\n    cp = vec3(0.2 * sin(time * 0.4), -0.02 + 0.2 * cos(time * 0.2), 1.5);\n    cu = vec3(sin(time * 0.8), 1.0, cos(time * 0.6));\n  }\n\n#if 0\n  if (iMouse.z > 0.0) // debug\n  {\n    float d = (iResolution.y-iMouse.y)*0.01+3.0;\n    cp = vec3(sin(iMouse.x*0.01)*d, .0, cos(iMouse.x*0.01)*d);\n  }\n#endif\n\n  vec3 cd = normalize(ct - cp);\n  vec3 cs = cross(cd, cu);\n  vec3 dir = normalize(iDir);\n\n  vec4 s;\n  float dist = 0.0;\n  vec3 ray = cp;\n\n  for (int i = 0; i < 40; i++) {\n    s = scene(ray);\n\n    dist += s.w;\n    ray += dir * s.w;\n\n    if (s.w < 0.01)\n      break;\n\n    if (dist > far) {\n      dist = far;\n      break;\n    }\n  }\n\n  float b = 1.0 - dist / far;\n  vec3 col = b * s.rgb;\n\n  // fake lights\n  col *= pow(1.3 + smoothstep(0.6, 0.0, abs(sin(ray.z + time * 2.0))), 2.0);\n\n  // vignetting & grain\n  col *= 1.0 - smoothstep(0.0, 2.0, length(pos.xy));\n  col -= 0.03 * rand(pos.xy);\n\n  gl_FragColor = vec4(col * 1.2, 1.0);\n}\n"
                    color: "white"
                    onCursorRectangleChanged: flick.ensureVisible(cursorRectangle)
                    selectByMouse: true
                    Settings {
                        category: "fontSize"
                        property alias fontSize: textEdit.font.pointSize
                    }
                }
            }
        }

        Component {
            id: textureViewBuilder
            Item {
                width: 128; height: 128


            }
        }


        Column {
            id: textureColumn
            anchors { right: parent.right; }
            spacing: 8
            Image {
                width: 128; height: 128;
                source: "../../presets/cube00_0.jpg"
            }
            Image {
                width: 128; height: 128;
                source: "../../presets/tex00.jpg"
            }
            Image {
                width: 128; height: 128;
                source: "../../presets/tex10.png"
            }
            Image {
                width: 128; height: 128;
                source: "../../presets/cube01_0.png"
            }

        }
    }
}
