import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts
import Hangar

// One project: what it is, why we think so, when it was touched, quick actions.
Rectangle {
    id: card

    required property string name
    required property string path
    required property string displayPath
    required property int category
    required property bool overridden
    required property var tags
    required property string reason
    required property var modified
    required property bool hasGit
    required property string branch
    required property string remoteUrl
    required property string editor

    signal openRequested()
    signal menuRequested(real x, real y)
    signal actionRequested(string action)

    readonly property bool hovered: hover.hovered
    readonly property color tint: Theme.categoryColor(category)

    radius: 14
    color: hovered ? Theme.cardHover : Theme.card
    border.width: 1
    border.color: hovered ? Theme.borderStrong : Theme.border
    Behavior on color { ColorAnimation { duration: 120 } }

    HoverHandler { id: hover }

    TapHandler {
        acceptedButtons: Qt.LeftButton
        onDoubleTapped: card.openRequested()
    }
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (point) => card.menuRequested(point.position.x, point.position.y)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Rectangle {
                width: 36
                height: 36
                radius: 10
                color: Theme.categoryTint(card.category)
                Icon {
                    anchors.centerIn: parent
                    name: Theme.categoryIcon(card.category)
                    size: 18
                    color: card.tint
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1
                Text {
                    Layout.fillWidth: true
                    text: card.name
                    color: Theme.text
                    font.pixelSize: 15
                    font.weight: Font.DemiBold
                    elide: Text.ElideRight
                }
                Text {
                    Layout.fillWidth: true
                    text: card.overridden ? Theme.categoryLabel(card.category) + " · выбрано вручную" : card.reason
                    color: Theme.textSecondary
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
            }
        }

        // Tags
        Flow {
            Layout.fillWidth: true
            Layout.preferredHeight: 22
            spacing: 5
            clip: true
            Repeater {
                model: card.tags.slice(0, 5)
                delegate: Rectangle {
                    required property string modelData
                    height: 20
                    width: tagText.implicitWidth + 14
                    radius: 6
                    color: Theme.chip
                    Text {
                        id: tagText
                        anchors.centerIn: parent
                        text: parent.modelData
                        color: Theme.textSecondary
                        font.pixelSize: 11
                    }
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

            // Meta (hidden behind the actions on hover)
            RowLayout {
                visible: !card.hovered
                Layout.fillWidth: true
                spacing: 6
                Text {
                    text: Theme.ago(card.modified)
                    color: Theme.textTertiary
                    font.pixelSize: 12
                }
                Rectangle {
                    visible: card.hasGit && card.branch.length > 0
                    height: 18
                    width: branchRow.implicitWidth + 12
                    radius: 5
                    color: "transparent"
                    border.width: 1
                    border.color: Theme.border
                    RowLayout {
                        id: branchRow
                        anchors.centerIn: parent
                        spacing: 3
                        Icon { name: "branch"; size: 11; color: Theme.textTertiary; strokeWidth: 2.2 }
                        Text {
                            text: card.branch
                            color: Theme.textTertiary
                            font.pixelSize: 11
                            font.family: Theme.monoFont
                        }
                    }
                }
                Item { Layout.fillWidth: true }
            }

            // Actions
            RowLayout {
                visible: card.hovered
                Layout.fillWidth: true
                spacing: 2

                AbstractButton {
                    id: openButton
                    implicitHeight: 28
                    implicitWidth: openLabel.implicitWidth + 30
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus
                    onClicked: card.openRequested()
                    background: Rectangle {
                        radius: 8
                        color: Qt.rgba(card.tint.r, card.tint.g, card.tint.b, openButton.hovered ? 0.32 : 0.2)
                    }
                    contentItem: RowLayout {
                        spacing: 5
                        Item { implicitWidth: 3 }
                        Icon { name: "code"; size: 13; color: Theme.text }
                        Text {
                            id: openLabel
                            text: card.editor.length ? card.editor.replace("Visual Studio Code", "VS Code") : "Открыть"
                            color: Theme.text
                            font.pixelSize: 12
                            font.weight: Font.DemiBold
                        }
                    }
                }
                Item { Layout.fillWidth: true }
                IconButton { iconName: "folder"; tip: "Показать в Finder"; onClicked: card.actionRequested("finder") }
                IconButton { iconName: "terminal"; tip: "Терминал здесь"; onClicked: card.actionRequested("terminal") }
                IconButton {
                    iconName: "trash"
                    tip: "Удалить…"
                    onClicked: card.actionRequested("trash")
                }
                IconButton {
                    visible: card.remoteUrl.length > 0
                    iconName: "external"
                    tip: "Открыть репозиторий"
                    onClicked: card.actionRequested("remote")
                }
            }
        }
    }
}
