import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root

    property string icon: "folder"
    property string title: ""
    property string subtitle: ""

    spacing: 10

    Rectangle {
        Layout.alignment: Qt.AlignHCenter
        width: 64
        height: 64
        radius: 18
        color: Theme.chip
        Icon {
            anchors.centerIn: parent
            name: root.icon
            size: 28
            color: Theme.textTertiary
        }
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.title
        color: Theme.text
        font.pixelSize: 17
        font.weight: Font.DemiBold
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        Layout.maximumWidth: 380
        text: root.subtitle
        color: Theme.textSecondary
        font.pixelSize: 13
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
        lineHeight: 1.2
    }
}
