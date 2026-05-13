pragma Singleton
import QtQuick

QtObject {
    id: utils

    function severityColor(s) {
        if (s === "critical") return "#f85149"
        if (s === "high")     return "#d29922"
        if (s === "medium")   return "#58a6ff"
        return "#3fb950"
    }

    function severityLabel(s) {
        if (s === "critical") return "КРИТИЧНО"
        if (s === "high")     return "ВЫСОКИЙ"
        if (s === "medium")   return "СРЕДНИЙ"
        return "НИЗКИЙ"
    }

    function formatTime(ts) {
        if (!ts) return ""
        return ts.replace("T", " ").substring(0, 19)
    }
}