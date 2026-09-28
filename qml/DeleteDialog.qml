import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Hangar

// "Move to Trash?" with a warning when the code exists nowhere else.
Popup {
    id: root

    required property AppController controller

    property string path: ""
    property string name: ""
    property bool hasGit: false
    property string remoteUrl: ""
    readonly property bool onlyCopy: remoteUrl.length === 0

    function ask(path, name, hasGit, remoteUrl) {
        root.path = path
        root.name = name
        root.hasGit = hasGit
        root.remoteUrl = remoteUrl
        open()
    }

    modal: true
    dim: true
    width: 420
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

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: "Удалить «" + root.name + "»?"
            color: Theme.text
            font.pixelSize: 18
            font.weight: Font.Bold
            elide: Text.ElideMiddle
        }
        Text {
            Layout.fillWidth: true
            text: "«В Корзину» удалит папку с диска (вернуть можно из Finder, пока Корзина не очищена). "
                + "«Только из списка» уберёт проект из Hangar, а папку не тронет."
            color: Theme.textSecondary
            font.pixelSize: 13
            wrapMode: Text.WordWrap
            lineHeight: 1.2
        }
        Text {
            Layout.fillWidth: true
            text: root.path
            color: Theme.textTertiary
            font.pixelSize: 12
            font.family: Theme.monoFont
            elide: Text.ElideMiddle
        }

        // Nothing on GitHub: this folder may be the only copy.
        Rectangle {
            visible: root.onlyCopy
            Layout.fillWidth: true
            implicitHeight: warnRow.implicitHeight + 20
            radius: 10
            color: Qt.rgba(1, 0.62, 0.04, Theme.dark ? 0.14 : 0.12)
            RowLayout {
                id: warnRow
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                Icon { name: "alert"; size: 16; color: "#ff9f0a"; Layout.alignment: Qt.AlignTop }
                Text {
                    Layout.fillWidth: true
                    text: root.hasGit ? "У проекта нет репозитория на GitHub: эта папка может быть единственной копией."
                                      : "Проект не в git и не на GitHub: эта папка — единственная копия кода."
                    color: Theme.text
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 6
            spacing: 8
            AbstractButton {
                id: forgetButton
                text: "Только из списка"
                implicitHeight: 34
                implicitWidth: forgetText.implicitWidth + 24
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root.controller.forget(root.path)
                    root.close()
                }
                background: Rectangle {
                    radius: 9
                    color: forgetButton.pressed ? Theme.selection : forgetButton.hovered ? Theme.hover : "transparent"
                    border.width: 1
                    border.color: Theme.border
                }
                contentItem: Text {
                    id: forgetText
                    text: forgetButton.text
                    color: Theme.text
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            Item { Layout.fillWidth: true }
            AbstractButton {
                id: cancel
                text: "Отмена"
                implicitHeight: 34
                implicitWidth: 90
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                onClicked: root.close()
                background: Rectangle {
                    radius: 9
                    color: cancel.pressed ? Theme.selection : cancel.hovered ? Theme.hover : "transparent"
                }
                contentItem: Text {
                    text: cancel.text
                    color: Theme.text
                    font.pixelSize: 13
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
            AbstractButton {
                id: confirm
                text: "В Корзину"
                implicitHeight: 34
                implicitWidth: 110
                hoverEnabled: true
                focusPolicy: Qt.NoFocus
                onClicked: {
                    root.controller.trash(root.path)
                    root.close()
                }
                background: Rectangle {
                    radius: 9
                    color: confirm.pressed ? "#d63a31" : confirm.hovered ? "#ff5a50" : "#ff453a"
                }
                contentItem: Text {
                    text: confirm.text
                    color: "white"
                    font.pixelSize: 13
                    font.weight: Font.DemiBold
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }
    }
}
