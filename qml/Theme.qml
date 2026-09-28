pragma Singleton
import QtQuick

// Design tokens, category metadata and formatting helpers.
QtObject {
    readonly property bool dark: Application.styleHints.colorScheme !== Qt.ColorScheme.Light

    // ---- surfaces
    readonly property color windowBg: dark ? "#16161a" : "#f5f5f7"
    readonly property color sidebarBg: dark ? "#1c1c21" : "#ececf0"
    readonly property color card: dark ? "#212127" : "#ffffff"
    readonly property color cardHover: dark ? "#27272e" : "#fbfbfd"
    readonly property color border: dark ? Qt.rgba(1, 1, 1, 0.08) : Qt.rgba(0, 0, 0, 0.09)
    readonly property color borderStrong: dark ? Qt.rgba(1, 1, 1, 0.16) : Qt.rgba(0, 0, 0, 0.18)
    readonly property color hover: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.045)
    readonly property color selection: dark ? Qt.rgba(1, 1, 1, 0.10) : Qt.rgba(0, 0, 0, 0.075)
    readonly property color field: dark ? Qt.rgba(1, 1, 1, 0.06) : Qt.rgba(0, 0, 0, 0.05)
    readonly property color chip: dark ? Qt.rgba(1, 1, 1, 0.07) : Qt.rgba(0, 0, 0, 0.05)
    readonly property color toastBg: dark ? "#3a3a42" : "#1c1c1f"

    // ---- text
    readonly property color text: dark ? "#f4f4f6" : "#18181b"
    readonly property color textSecondary: dark ? Qt.rgba(1, 1, 1, 0.60) : Qt.rgba(0, 0, 0, 0.58)
    readonly property color textTertiary: dark ? Qt.rgba(1, 1, 1, 0.38) : Qt.rgba(0, 0, 0, 0.38)

    readonly property color accent: "#7c5cff"
    readonly property color accentText: dark ? "#bcaeff" : "#5a3fe0"
    readonly property string monoFont: Qt.platform.os === "osx" ? "Menlo" : "monospace"

    // ---- categories (same order as hangar::Category)
    readonly property var _names: ["Мини-аппы", "Боты", "Сайты", "Приложения", "Игры", "Скрипты", "Учёба", "Архив"]
    readonly property var _single: ["Мини-апп", "Бот", "Сайт", "Приложение", "Игра", "Скрипт", "Учёба", "Архив"]
    readonly property var _icons: ["smartphone", "send", "globe", "monitor", "gamepad", "terminal", "book", "archive"]
    readonly property var _rgb: [[0.49, 0.36, 1.0], [0.16, 0.67, 0.93], [0.19, 0.82, 0.35], [1.0, 0.62, 0.04],
                                 [1.0, 0.22, 0.37], [0.39, 0.82, 1.0], [1.0, 0.84, 0.04], [0.56, 0.56, 0.60]]
    readonly property int categoryCount: 8

    function categoryName(c) { return c < 0 ? "Все проекты" : (_names[c] ?? "") }
    function categoryLabel(c) { return _single[c] ?? "" }
    function categoryIcon(c) { return c < 0 ? "grid" : (_icons[c] ?? "folder") }
    function categoryColor(c) {
        if (c < 0) return accent
        const v = _rgb[c] ?? [0.5, 0.5, 0.5]
        return Qt.rgba(v[0], v[1], v[2], 1)
    }
    function categoryTint(c) {
        const v = c < 0 ? [0.49, 0.36, 1.0] : (_rgb[c] ?? [0.5, 0.5, 0.5])
        return Qt.rgba(v[0], v[1], v[2], dark ? 0.18 : 0.13)
    }

    // ---- formatting
    function plural(n, one, few, many) {
        const m10 = n % 10, m100 = n % 100
        if (m10 === 1 && m100 !== 11) return one
        if (m10 >= 2 && m10 <= 4 && (m100 < 10 || m100 >= 20)) return few
        return many
    }

    function ago(d) {
        if (!d || isNaN(d.getTime())) return ""
        const sec = (Date.now() - d.getTime()) / 1000
        if (sec < 90) return "только что"
        const min = Math.floor(sec / 60)
        if (min < 60) return min + " " + plural(min, "минуту", "минуты", "минут") + " назад"
        const h = Math.floor(min / 60)
        if (h < 24) return h + " " + plural(h, "час", "часа", "часов") + " назад"
        const days = Math.floor(h / 24)
        if (days === 1) return "вчера"
        if (days < 30) return days + " " + plural(days, "день", "дня", "дней") + " назад"
        const months = Math.floor(days / 30)
        if (months < 12) return months + " " + plural(months, "месяц", "месяца", "месяцев") + " назад"
        const years = Math.floor(days / 365)
        return years + " " + plural(years, "год", "года", "лет") + " назад"
    }

    function projectsWord(n) { return plural(n, "проект", "проекта", "проектов") }
}
