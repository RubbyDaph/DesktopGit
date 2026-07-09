import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: control

    property bool primary: false
    property string iconName: ""

    implicitWidth: Math.max(92, buttonContent.implicitWidth + leftPadding + rightPadding)
    implicitHeight: 34
    leftPadding: 14
    rightPadding: 14
    topPadding: 7
    bottomPadding: 7

    hoverEnabled: true

    contentItem: RowLayout {
        id: buttonContent

        spacing: control.iconName.length > 0 ? 7 : 0

        AppIcon {
            id: buttonIcon

            name: control.iconName
            size: 15
            opacity: control.enabled ? 1.0 : 0.45
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: control.text
            color: control.enabled
                ? (control.primary ? "#ffffff" : "#e7e9ee")
                : "#707782"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            font.pixelSize: 13
            font.weight: control.primary ? Font.DemiBold : Font.Medium
            Layout.alignment: Qt.AlignVCenter
            Layout.fillWidth: true
        }
    }

    background: Rectangle {
        radius: 6
        color: {
            if (!control.enabled) {
                return "#24262a"
            }

            if (control.primary) {
                return control.down ? "#5d7fc9" : (control.hovered ? "#789bea" : "#6f93dd")
            }

            return control.down ? "#34373d" : (control.hovered ? "#30333a" : "#2b2d31")
        }
        border.color: control.primary
            ? (control.enabled ? "#8cabf0" : "#3a3d42")
            : (control.hovered ? "#565b64" : "#3a3d42")
        border.width: 1
    }
}
