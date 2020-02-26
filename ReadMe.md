## Intro

Screen QR code scanning utility for Windows.

## Rec

![rec](ReadMe/rec.gif)

## Release

Please refer to the *release* page.

## Configuration

Color scheme and key bindings can be configured via a json formatted config file:

```shell
./QRUtil [config_filename] (default: ./config.json).
```

**Example**

```json
{
    "COLOR":{
       "T_DARK":"46000000",
       "T_QR_BOARDER":"FF007ACC",
       "BOARDER_THICKNESS":2.0, 
       "T_QR_NO_HOVER":"1E007ACC",
       "T_QR_HOVER":"64007ACC",
       "QR_TEXT_BACKGROUND":"50000000",
       "QR_TEXT_COLOR":"FFFFFFFF"
    },
    "HOTKEY":{ 
       "CTRL":true,
       "ALT":true,
       "SHIFT":true,
       "VKEY":"I"
    },
    "KEY":{
       "SELECT_ALL":"A",
       "DESELECT_ALL":"D",
       "COPY_SELECTED":"C",
       "EXIT_PROGRAM": "71"
    }
 }
```

For detailed explanations of the parameters, refer to the comments in [config.json](./QRUtil/config.json).

## Build

**Install Dependencies**

- [opencv-4.2.0-vc15](https://sourceforge.net/projects/opencvlibrary/files/4.2.0/opencv-4.2.0-vc14_vc15.exe/download)
- [zbar-0.10-windows-installer](http://sourceforge.net/projects/zbar/files/zbar/0.10/zbar-0.10-setup.exe/download)
- [jsoncpp](https://github.com/open-source-parsers/jsoncpp)

**Set Environment Variables**

- OPENCV_DIR to ...\build\x64\vc15
- ZBAR_DIR to the installation directory

Copy x64 libs from [ZBarWin64](https://github.com/dani4/ZBarWin64/tree/master/lib) to <ZBAR_DIR>\lib.

**Build**

Open `Proj.sln` with Visual Studio 2019 and go building.

## Todo

- Support decoding on a manually selected range when auto-detection's got a problem.

## License

[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html)

