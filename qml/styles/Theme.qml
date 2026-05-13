import QtQuick

QtObject {
    readonly property color bgPrimary:    "#0d1117"
    readonly property color bgSecondary:  "#161b22"
    readonly property color bgTertiary:   "#21262d"
    readonly property color border:       "#30363d"
    readonly property color borderHover:  "#58a6ff"

    readonly property color textPrimary:  "#e6edf3"
    readonly property color textSecondary:"#8b949e"
    readonly property color textMuted:    "#484f58"

    readonly property color accent:       "#58a6ff"
    readonly property color accentHover:  "#79c0ff"
    readonly property color accentDark:   "#1f6feb"

    readonly property color success:      "#3fb950"
    readonly property color successDark:  "#238636"
    readonly property color danger:       "#f85149"
    readonly property color dangerDark:   "#da3633"
    readonly property color warning:      "#d29922"

    readonly property color roleAdmin:    "#f85149"
    readonly property color roleOperator: "#d29922"
    readonly property color roleViewer:   "#58a6ff"

    // Радиусы
    readonly property int radiusSmall:  4
    readonly property int radiusMedium: 6
    readonly property int radiusLarge:  12  
    readonly property int radiusXL:     16
    readonly property int radiusFull:   9999

    // Шрифты
    readonly property int fontSizeXS:   11
    readonly property int fontSizeSM:   12
    readonly property int fontSizeMD:   14
    readonly property int fontSizeLG:   16
    readonly property int fontSizeXL:   20
    readonly property int fontSizeXXL:  24

    readonly property string fontFamily: "Segoe UI"

    // Анимации
    readonly property int animFast:   150
    readonly property int animNormal: 200
    readonly property int animSlow:   300
}