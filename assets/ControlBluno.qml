/**

 Control Bluno

**/

import QtQuick 2.12
import QtQuick.Controls 2.12 as QtControls

Rectangle {
    width: 300
    height: 600

    Header {
        id: header
        anchors.top: parent.top
        headerText: "Control Bluno"
    }

    Connections {
        target: device
        onCharacteristicsUpdated: {
            menu.menuText = "Back";
	    print('ControlBluno Connections')
        }

        onDisconnected: {
	    print('Bij disconnecting ...');
            pageLoader.source = "main.qml"
        }
    }

    Row {
	id: rowid
	spacing: 2
	anchors.top: header.bottom
	QtControls.TextField {
            id: command
	    placeholderText: qsTr("Enter command")
	}
	QtControls.Button {
	    text: 'Send'
	    onClicked: device.command = command.text
	}
    }

    // from bluno
    Row {
	id: rowid2
	spacing: 2
	anchors.top: rowid.bottom
	Text {
	    color: 'red'
	    text: device.bluno
	}
    }

    Menu {
        id: menu
        anchors.bottom: parent.bottom
        menuWidth: parent.width
        menuHeight: (parent.height/6)
        menuText: device.update
        onButtonClick: {
            pageLoader.source = "Characteristics.qml"
            device.update = "Back from control"
        }
    }
}
