import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Hangar

// "Sort into folders": explains what will happen, shows the plan, runs it.
Popup {
    id: root

    required property AppController controller

    modal: true
    dim: true
    width: 460
    padding: 24
    anchors.centerIn: Overlay.overlay
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.45) }

    background: Rectangle {
        radius: 16
        color: Theme.card
        border.width: 1
        border.color: Theme.borderStrong
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 140 }
        NumberAnimation { property: "scale"; from: 0.96; to: 1; duration: 180; easing.type: Easing.OutCubic }
    }

    FolderDialog {
        id: targetPicker
        title: "Куда складывать папки"
        onAccepted: root.controller.organizeRoot = selectedFolder
    }

    contentItem: ColumnLayout {
        spacing: 14

        Text {
            text: "Разложить по папкам"
            color: Theme.text
            font.pixelSize: 19
            font.weight: Font.Bold
        }
        Text {
            Layout.fillWidth: true
            text: "В папке ниже появятся «Боты», «Сайты», «Мини-аппы»… с ярлыками на проекты. "
                + "Сами проекты остаются на месте, поэтому .venv, сборки CMake и списки в IDE не сломаются."
            color: Theme.textSecondary
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            lineHeight: 1.2
        }

        // Target
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 42
            radius: 10
            color: Theme.field
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 6
                spacing: 8
                Icon { name: "folder"; size: 15; color: Theme.textSecondary }
                Text {
                    Layout.fillWidth: true
                    text: root.controller.organizeRootDisplay
                    color: Theme.text
                    font.pixelSize: 13
                    font.family: Theme.monoFont
                    elide: Text.ElideMiddle
                }
                FlatButton {
                    text: "Изменить"
                    onClicked: targetPicker.open()
                }
            }
        }

        // Plan
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            rowSpacing: 6
            columnSpacing: 16
            Repeater {
                model: Theme.categoryCount
                delegate: RowLayout {
                    required property int index
                    readonly property int n: root.controller.model.counts[index] ?? 0
                    visible: n > 0
                    Layout.fillWidth: true
                    spacing: 8
                    Icon { name: Theme.categoryIcon(index); size: 14; color: Theme.categoryColor(index) }
                    Text {
                        Layout.fillWidth: true
                        text: Theme.categoryName(index)
                        color: Theme.text
                        font.pixelSize: 13
                    }
                    Text {
                        text: n
                        color: Theme.textTertiary
                        font.pixelSize: 13
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 8
            FlatButton {
                text: "Открыть папку"
                onClicked: root.controller.revealOrganized()
            }
            Item { Layout.fillWidth: true }
            FlatButton {
                text: "Отмена"
                onClicked: root.close()
            }
            Button {
                id: go
                text: "Разложить"
                highlighted: true
                onClicked: {
                    root.controller.organize()
                    root.close()
                    root.controller.revealOrganized()
                }
                background: Rectangle {
                    implicitWidth: 110
                    implicitHeight: 34
                    radius: 9
                    color: go.pressed ? Qt.darker(Theme.accent, 1.15) : go.hovered ? Qt.lighter(Theme.accent, 1.08) : Theme.accent
                }
                contentItem: Text {
                    text: go.text
                    color: "white"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }

    component FlatButton: AbstractButton {
        id: flat
        implicitHeight: 32
        implicitWidth: flatText.implicitWidth + 24
        hoverEnabled: true
        focusPolicy: Qt.NoFocus
        background: Rectangle {
            radius: 8
            color: flat.pressed ? Theme.selection : flat.hovered ? Theme.hover : "transparent"
        }
        contentItem: Text {
            id: flatText
            text: flat.text
            color: Theme.text
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
