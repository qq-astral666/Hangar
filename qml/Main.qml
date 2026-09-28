import QtQml
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts
import Hangar

// Sidebar with categories and folders; a grid of project cards; drop a
// folder anywhere on the window to add it.
ApplicationWindow {
    id: win

    required property AppController controller

    width: 1180
    height: 760
    minimumWidth: 820
    minimumHeight: 520
    visible: true
    title: "Hangar"
    color: Theme.windowBg

    readonly property var model: controller.model
    // Card the context menu was opened for.
    property var menuTarget: null

    Connections {
        target: win.controller
        function onToast(message) { toast.show(message) }
        function onFindRequested() { search.forceActiveFocus(); search.selectAll() }
    }

    FolderDialog {
        id: addPicker
        title: "Папка с проектами или проект"
        onAccepted: win.controller.addUrls([selectedFolder])
    }

    OrganizeDialog {
        id: organizeDialog
        controller: win.controller
    }

    DeleteDialog {
        id: deleteDialog
        controller: win.controller
    }

    // ⌘R / ⌘F are handled in C++ (main.cpp) by physical key, so they also
    // work with the Russian layout.

    RowLayout {
        anchors.fill: parent
        spacing: 0

        Sidebar {
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            controller: win.controller
            onAddFolderRequested: addPicker.open()
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // ---- header
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 18
                Layout.leftMargin: 24
                Layout.rightMargin: 20
                Layout.bottomMargin: 14
                spacing: 12

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: Theme.categoryName(win.model.category)
                        color: Theme.text
                        font.pixelSize: 22
                        font.weight: Font.Bold
                    }
                    Text {
                        text: win.model.count + " " + Theme.projectsWord(win.model.count)
                              + (win.controller.scanning ? " · обновляю…" : "")
                        color: Theme.textTertiary
                        font.pixelSize: 12
                    }
                }

                Item { Layout.fillWidth: true }

                // Search
                Rectangle {
                    Layout.preferredWidth: 260
                    implicitHeight: 34
                    radius: 9
                    color: Theme.field
                    border.width: search.activeFocus ? 1 : 0
                    border.color: Theme.accent
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 10
                        anchors.rightMargin: 4
                        spacing: 6
                        Icon { name: "search"; size: 14; color: Theme.textTertiary }
                        TextField {
                            id: search
                            Layout.fillWidth: true
                            placeholderText: "Поиск: имя, aiogram, React…"
                            placeholderTextColor: Theme.textTertiary
                            color: Theme.text
                            font.pixelSize: 13
                            background: null
                            leftPadding: 0
                            onTextChanged: win.model.search = text
                            Keys.onEscapePressed: { text = ""; focus = false }
                        }
                        IconButton {
                            visible: search.text.length > 0
                            implicitWidth: 24
                            implicitHeight: 24
                            iconName: "x"
                            iconSize: 12
                            onClicked: search.text = ""
                        }
                    }
                }

                IconButton {
                    iconName: "refresh"
                    tip: "Обновить (⌘R)"
                    RotationAnimation on rotation {
                        running: win.controller.scanning
                        loops: Animation.Infinite
                        from: 0; to: 360; duration: 900
                    }
                    onClicked: win.controller.rescan()
                }

                AbstractButton {
                    id: organizeButton
                    implicitHeight: 34
                    implicitWidth: organizeRow.implicitWidth + 26
                    hoverEnabled: true
                    focusPolicy: Qt.NoFocus
                    enabled: win.model.total > 0
                    opacity: enabled ? 1 : 0.4
                    onClicked: organizeDialog.open()
                    background: Rectangle {
                        radius: 9
                        color: organizeButton.pressed ? Qt.darker(Theme.accent, 1.15)
                             : organizeButton.hovered ? Qt.lighter(Theme.accent, 1.08) : Theme.accent
                    }
                    contentItem: RowLayout {
                        id: organizeRow
                        spacing: 7
                        Item { implicitWidth: 4 }
                        Icon { name: "grid"; size: 14; color: "white" }
                        Text {
                            text: "Разложить по папкам"
                            color: "white"
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }
                    }
                }
            }

            // ---- grid
            GridView {
                id: grid
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.leftMargin: 18
                Layout.rightMargin: 12
                clip: true
                model: win.model
                boundsBehavior: Flickable.StopAtBounds

                readonly property int columns: Math.max(1, Math.floor(width / 300))
                cellWidth: Math.floor(width / columns)
                cellHeight: 168

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: Item {
                    id: cell
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
                    required property var editors

                    width: grid.cellWidth
                    height: grid.cellHeight

                    ProjectCard {
                        anchors.fill: parent
                        anchors.margins: 6
                        name: cell.name
                        path: cell.path
                        displayPath: cell.displayPath
                        category: cell.category
                        overridden: cell.overridden
                        tags: cell.tags
                        reason: cell.reason
                        modified: cell.modified
                        hasGit: cell.hasGit
                        branch: cell.branch
                        remoteUrl: cell.remoteUrl
                        editor: cell.editor

                        onOpenRequested: win.controller.openIn(cell.path, cell.editor)
                        onActionRequested: (action) => {
                            if (action === "finder") win.controller.reveal(cell.path)
                            else if (action === "terminal") win.controller.openTerminal(cell.path)
                            else if (action === "remote") win.controller.openUrl(cell.remoteUrl)
                            else if (action === "trash") deleteDialog.ask(cell.path, cell.name, cell.hasGit, cell.remoteUrl)
                        }
                        onMenuRequested: (x, y) => {
                            win.menuTarget = {
                                path: cell.path, name: cell.name, category: cell.category, hasGit: cell.hasGit,
                                overridden: cell.overridden, editors: cell.editors, remoteUrl: cell.remoteUrl
                            }
                            const p = mapToItem(win.contentItem, x, y)
                            projectMenu.popup(win.contentItem, p.x, p.y)
                        }
                    }
                }

                EmptyState {
                    anchors.centerIn: parent
                    visible: grid.count === 0 && !win.controller.scanning
                    icon: win.model.total === 0 ? "drop" : "search"
                    title: win.model.total === 0 ? "Перетащи сюда папку с проектами" : "Ничего не нашлось"
                    subtitle: win.model.total === 0
                        ? "Например, CLionProjects или PycharmProjects. Hangar сам поймёт, где бот, где сайт, а где лаба."
                        : "Попробуй другое слово или выбери «Все проекты»."
                }
            }
        }
    }

    // ---- context menu
    Menu {
        id: projectMenu

        MenuItem {
            text: "Открыть в Finder"
            onTriggered: win.controller.reveal(win.menuTarget.path)
        }
        MenuItem {
            text: "Открыть в Терминале"
            onTriggered: win.controller.openTerminal(win.menuTarget.path)
        }
        MenuItem {
            text: "Открыть репозиторий"
            enabled: win.menuTarget !== null && win.menuTarget.remoteUrl.length > 0
            onTriggered: win.controller.openUrl(win.menuTarget.remoteUrl)
        }
        MenuItem {
            text: "Скопировать путь"
            onTriggered: win.controller.copyPath(win.menuTarget.path)
        }
        MenuSeparator {}
        MenuItem {
            text: "Убрать из списка"
            onTriggered: win.controller.forget(win.menuTarget.path)
        }
        MenuItem {
            text: "Удалить…"
            onTriggered: deleteDialog.ask(win.menuTarget.path, win.menuTarget.name,
                                          win.menuTarget.hasGit, win.menuTarget.remoteUrl)
        }
        MenuSeparator {}

        Menu {
            id: editorsMenu
            title: "Открыть в…"
            Instantiator {
                model: win.menuTarget ? win.menuTarget.editors : []
                delegate: MenuItem {
                    required property string modelData
                    text: modelData
                    onTriggered: win.controller.openIn(win.menuTarget.path, modelData)
                }
                onObjectAdded: (index, object) => editorsMenu.insertItem(index, object)
                onObjectRemoved: (index, object) => editorsMenu.removeItem(object)
            }
        }

        Menu {
            id: categoryMenu
            title: "Категория"
            MenuItem {
                text: "Автоматически"
                checkable: true
                checked: win.menuTarget !== null && !win.menuTarget.overridden
                onTriggered: win.controller.setCategory(win.menuTarget.path, -1)
            }
            MenuSeparator {}
            Instantiator {
                model: Theme.categoryCount
                delegate: MenuItem {
                    required property int index
                    text: Theme.categoryName(index)
                    checkable: true
                    checked: win.menuTarget !== null && win.menuTarget.overridden && win.menuTarget.category === index
                    onTriggered: win.controller.setCategory(win.menuTarget.path, index)
                }
                onObjectAdded: (index, object) => categoryMenu.insertItem(index + 2, object)
                onObjectRemoved: (index, object) => categoryMenu.removeItem(object)
            }
        }
    }

    // ---- drop anywhere
    DropArea {
        id: drop
        anchors.fill: parent
        keys: ["text/uri-list"]
        onDropped: (event) => {
            if (event.hasUrls)
                win.controller.addUrls(event.urls)
        }

        Rectangle {
            anchors.fill: parent
            anchors.margins: 10
            visible: drop.containsDrag
            radius: 18
            color: Qt.rgba(0.49, 0.36, 1, 0.12)
            border.width: 2
            border.color: Theme.accent

            EmptyState {
                anchors.centerIn: parent
                icon: "drop"
                title: "Отпусти, чтобы добавить"
                subtitle: "Папку с проектами — буду следить за ней. Один проект — добавлю его."
            }
        }
    }

    // ---- toast
    Rectangle {
        id: toast

        function show(message) {
            toastText.text = message
            opacity = 1
            toastTimer.restart()
        }

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 24
        width: toastText.implicitWidth + 32
        height: 38
        radius: 19
        color: Theme.toastBg
        opacity: 0
        visible: opacity > 0
        Behavior on opacity { NumberAnimation { duration: 180 } }

        Text {
            id: toastText
            anchors.centerIn: parent
            color: "white"
            font.pixelSize: 13
        }
        Timer {
            id: toastTimer
            interval: 2600
            onTriggered: toast.opacity = 0
        }
    }
}
