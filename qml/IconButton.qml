import QtQuick
import QtQuick.Controls.Basic

// Small square button with an icon and a tooltip.
AbstractButton {
    id: root

    property string iconName: ""
    property string tip: ""
    property color iconColor: Theme.textSecondary
    property real iconSize: 16

    implicitWidth: 30
    implicitHeight: 30
    hoverEnabled: true
    focusPolicy: Qt.NoFocus

    background: Rectangle {
        radius: 8
        color: root.pressed ? Theme.selection : root.hovered ? Theme.hover : "transparent"
    }
    contentItem: Item {
        Icon {
            anchors.centerIn: parent
            name: root.iconName
            size: root.iconSize
            color: root.hovered ? Theme.text : root.iconColor
        }
    }

    ToolTip.visible: hovered && tip.length > 0
    ToolTip.delay: 500
    ToolTip.text: tip
}
