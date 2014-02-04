#TinySWF - Tiny SWF player
![TinySWF - Tiny SWF player](https://dl.dropboxusercontent.com/u/2907407/tinyswf.png)

**TinySWF is a simple SWF (Adobe Flash) player which can play FLASH contents in cocos2d-x and OpenGL environment.**

Creating 2D user interfaces requires a good graphics editor which needed lot of time and resource to develop.
Adobe Flash editor already provides developers a good editing environment to create user interfaces and animations.
TinySWF directly parse and play FLASH contents to speed up development processes.

![sample](https://dl.dropboxusercontent.com/u/2907407/tinyswf-sample.png)


Features
----------

* Bitmap
* Motion Tween
* Text
* Shape
* Button
* MovieClip
* Mask
* Sound
* SWF3 action model 
    - play
    - stop
    - gotoAndPlay
    - nextFrame
    - fscommand


First step
----------

1. Download source code from [Github][1]
2. Enter tsviewer/porj.vc11
3. Open tsviewer.sln by vs2012

TSViewer is a simple FLASH viewer which also provides command line tool to trim unsupported tags in FLASH contents to save space of SWF files.

Project Dpendence:

* [FreeGLUT][2]
* [SOIL][3]
* [FMOD][4] (Optional)


How to use TinySWF in cocos2d-x
----------

1. Download source code from [Github][1]
2. Enter cocos2d_swf/porj.win32
3. Open HelloCpp.sln by vs2012

HelloCpp is a simple FLASH viewer by coco2d-x.

Project Dpendence:

* [cocos2d-x 3.0 alpha][5]


Test data
----------
test folder contains some FLASH data for testing.


Bitmap Usage
----------
For optimization reason, TinySWF loads outside texture files instead of using embbed bitmap in SWF files.

Please read [bitmap guide][6].


----------

[1]: http://github.com/jbyu/tinyswf
[2]: http://freeglut.sourceforge.net
[3]: http://www.lonesock.net/soil.html
[4]: http://www.fmod.org/
[5]: http://www.cocos2d-x.org
[6]: https://dl.dropboxusercontent.com/u/2907407/tinyswf-bitmap.pdf
