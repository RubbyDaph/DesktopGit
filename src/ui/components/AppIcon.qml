import QtQuick

Image {
    id: root

    property string name
    property int size: 16

    visible: name.length > 0
    source: name.length > 0 ? "qrc:/assets/icons/lucide/" + name + ".svg" : ""
    sourceSize.width: size
    sourceSize.height: size
    fillMode: Image.PreserveAspectFit
    smooth: true
    width: size
    height: size
}
