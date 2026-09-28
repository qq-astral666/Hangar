import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Hangar

// Categories on top, watched folders below.
Rectangle {
    id: root

    required property AppController controller
    signal addFolderRequested()

    color: Theme.sidebarBg

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.topMargin: 16
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        anchors.bottomMargin: 12
        spacing: 2

        Text {
            Layout.leftMargin: 8
            Layout.bottomMargin: 10
            text: "Hangar"
            color: Theme.text
            font.pixelSize: 20
            font.weight: Font.Bold
        }

        Repeater {
            model: Theme.categoryCount + 1
            delegate: SidebarRow {
                required property int index
                readonly property int cat: index - 1
                readonly property int n: cat < 0 ? root.controller.model.total
                                                 : (root.controller.model.counts[cat] ?? 0)
                visible: cat < 0 || n > 0
                iconName: Theme.categoryIcon(cat)
                iconColor: Theme.categoryColor(cat)
                label: Theme.categoryName(cat)
                count: n
                selected: root.controller.model.category === cat
                onClicked: root.controller.model.category = cat
            }
        }

        Text {
            Layout.leftMargin: 8
            Layout.topMargin: 20
            Layout.bottomMargin: 4
            text: "ПАПКИ"
            color: Theme.textTertiary
            font.pixelSize: 11
            font.weight: Font.DemiBold
            font.letterSpacing: 0.8
        }

        ListView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.controller.roots
            boundsBehavior: Flickable.StopAtBounds
            delegate: Item {
                id: folderRow
                required property string modelData
                width: ListView.view.width
                height: 30

                HoverHandler { id: rowHover }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    spacing: 8
                    Icon {
                        name: "folder"
                        size: 14
                        color: Theme.textTertiary
                    }
                    Text {
                        Layout.fillWidth: true
                        text: folderRow.modelData.split("/").pop()
                        color: Theme.textSecondary
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }
                    IconButton {
                        visible: rowHover.hovered
                        implicitWidth: 24
                        implicitHeight: 24
                        iconName: "x"
                        iconSize: 12
                        tip: "Не следить за папкой"
                        onClicked: root.controller.removeRoot(folderRow.modelData)
                    }
                }
            }
        }

        SidebarRow {
            visible: root.controller.hiddenCount > 0
            iconName: "archive"
            iconColor: Theme.textTertiary
            label: "Вернуть скрытые"
            count: root.controller.hiddenCount
            onClicked: root.controller.showHidden()
        }

        SidebarRow {
            iconName: "folderPlus"
            iconColor: Theme.textSecondary
            label: "Добавить папку"
            count: -1
            onClicked: root.addFolderRequested()
        }
    }

    component SidebarRow: AbstractButton {
        id: row

        property string iconName
        property color iconColor
        property string label
        property int count: 0
        property bool selected: false

        Layout.fillWidth: true
        implicitHeight: 32
        hoverEnabled: true
        focusPolicy: Qt.NoFocus

        background: Rectangle {
            radius: 8
            color: row.selected ? Theme.selection : row.hovered ? Theme.hover : "transparent"
        }
        contentItem: RowLayout {
            spacing: 10
            Item { implicitWidth: 0 }
            Icon {
                name: row.iconName
                size: 16
                color: row.iconColor
            }
            Text {
                Layout.fillWidth: true
                text: row.label
                color: row.selected ? Theme.text : Theme.textSecondary
                font.pixelSize: 13
                font.weight: row.selected ? Font.DemiBold : Font.Normal
                elide: Text.ElideRight
            }
            Text {
                visible: row.count >= 0
                text: row.count
                color: Theme.textTertiary
                font.pixelSize: 12
            }
            Item { implicitWidth: 2 }
        }
    }
}
