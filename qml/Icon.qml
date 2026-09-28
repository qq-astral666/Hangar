import QtQuick
import QtQuick.Shapes

// Stroke icons from SVG path data (24×24 grid, Lucide-style): no image assets.
Item {
    id: root

    property string name: ""
    property color color: "white"
    property real size: 16
    property real strokeWidth: 1.9

    implicitWidth: size
    implicitHeight: size
    width: size
    height: size

    readonly property var paths: ({
        "search": "M4 11a7 7 0 1 0 14 0a7 7 0 1 0 -14 0 M20 20l-4.35 -4.35",
        "grid": "M4 4h6v6H4z M14 4h6v6h-6z M4 14h6v6H4z M14 14h6v6h-6z",
        "smartphone": "M7 2h10a2 2 0 0 1 2 2v16a2 2 0 0 1 -2 2H7a2 2 0 0 1 -2 -2V4a2 2 0 0 1 2 -2z M11 18h2",
        "send": "M22 2L11 13 M22 2l-7 20l-4 -9l-9 -4z",
        "globe": "M2 12a10 10 0 1 0 20 0a10 10 0 1 0 -20 0 M2 12h20 M12 2a15.3 15.3 0 0 1 4 10a15.3 15.3 0 0 1 -4 10a15.3 15.3 0 0 1 -4 -10a15.3 15.3 0 0 1 4 -10z",
        "monitor": "M4 3h16a2 2 0 0 1 2 2v10a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V5a2 2 0 0 1 2 -2z M8 21h8 M12 17v4",
        "gamepad": "M6 12h4 M8 10v4 M15 13h.01 M18 11h.01 M17.3 5H6.7a4 4 0 0 0 -3.98 3.59l-.6 6A3 3 0 0 0 5.1 18c1 0 1.9 -.55 2.35 -1.42L8.5 15h7l1.05 1.58A2.6 2.6 0 0 0 18.9 18a3 3 0 0 0 2.98 -3.41l-.6 -6A4 4 0 0 0 17.3 5z",
        "terminal": "M4 17l6 -6l-6 -6 M12 19h8",
        "book": "M4 19.5A2.5 2.5 0 0 1 6.5 17H20 M6.5 2H20v20H6.5A2.5 2.5 0 0 1 4 19.5v-15A2.5 2.5 0 0 1 6.5 2z",
        "archive": "M3 3h18v5H3z M5 8v12h14V8 M10 12h4",
        "folder": "M4 4h5l2 3h9a2 2 0 0 1 2 2v9a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V6a2 2 0 0 1 2 -2z",
        "folderPlus": "M4 4h5l2 3h9a2 2 0 0 1 2 2v9a2 2 0 0 1 -2 2H4a2 2 0 0 1 -2 -2V6a2 2 0 0 1 2 -2z M12 11v6 M9 14h6",
        "plus": "M12 5v14 M5 12h14",
        "x": "M18 6L6 18 M6 6l12 12",
        "refresh": "M21 12a9 9 0 1 1 -2.64 -6.36L21 8 M21 3v5h-5",
        "branch": "M6 3v12 M18 9a3 3 0 1 0 0 -6a3 3 0 0 0 0 6z M6 21a3 3 0 1 0 0 -6a3 3 0 0 0 0 6z M18 9a9 9 0 0 1 -9 9",
        "external": "M15 3h6v6 M10 14L21 3 M18 13v6a2 2 0 0 1 -2 2H5a2 2 0 0 1 -2 -2V8a2 2 0 0 1 2 -2h6",
        "code": "M16 18l6 -6l-6 -6 M8 6l-6 6l6 6",
        "sort": "M3 6h18 M6 12h12 M10 18h4",
        "check": "M5 12l5 5L20 7",
        "trash": "M3 6h18 M8 6V4h8v2 M6 6l1 14h10l1 -14 M10 11v6 M14 11v6",
        "alert": "M12 9v4 M12 17h.01 M10.3 3.9L1.8 18a2 2 0 0 0 1.7 3h17a2 2 0 0 0 1.7 -3L13.7 3.9a2 2 0 0 0 -3.4 0z",
        "link": "M10 13a5 5 0 0 0 7.54 .54l3 -3a5 5 0 0 0 -7.07 -7.07l-1.72 1.71 M14 11a5 5 0 0 0 -7.54 -.54l-3 3a5 5 0 0 0 7.07 7.07l1.71 -1.71",
        "drop": "M12 3v12 M7 10l5 5l5 -5 M4 17v2a2 2 0 0 0 2 2h12a2 2 0 0 0 2 -2v-2"
    })

    Shape {
        width: 24
        height: 24
        scale: root.size / 24
        transformOrigin: Item.TopLeft
        preferredRendererType: Shape.CurveRenderer

        ShapePath {
            strokeColor: root.color
            strokeWidth: root.strokeWidth
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap
            joinStyle: ShapePath.RoundJoin
            PathSvg { path: root.paths[root.name] ?? "" }
        }
    }
}
