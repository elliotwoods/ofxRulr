Rulr
====

A node based modular toolkit for calibration of spatial artworks featuring:

	* 3D view + Node based view
	* Standardised models for cameras, projectors
	* Many calibration routines
	* Saving / Loading / Exporting
	* Dynamic linking

# Requirements

## Addons

* https://github.com/elliotwoods/ofxClipboard.git
* https://github.com/arturoc/ofxHttpServer.git
* https://github.com/elliotwoods/ofxCvGui.git
* https://github.com/elliotwoods/ofxCvMin.git
* https://github.com/elliotwoods/ofxPlugin
* https://github.com/elliotwoods/ofxSingleton.git
* https://github.com/elliotwoods/ofxAssets.git
* https://github.com/elliotwoods/ofxGrabCam.git
* https://github.com/elliotwoods/ofxMachineVision.git
* https://github.com/elliotwoods/ofxWebWidgets.git
* https://github.com/elliotwoods/ofxWebWidgets.git
* https://github.com/elliotwoods/ofxRay.git
* https://github.com/elliotwoods/ofxNonLinearFit.git
* https://github.com/elliotwoods/ofxMessagePack.git
* https://github.com/elliotwoods/ofxSpout.git
* ofxJSON (core)
* ofxOSC (core)

## Addons for recommended plugins

* https://github.com/elliotwoods/ofxKinectForWindows2.git
* https://github.com/elliotwoods/ofxArUco.git
* https://github.com/elliotwoods/ofxBlackmagic2.git
* https://github.com/elliotwoods/ofxCanon.git
* https://github.com/elliotwoods/ofxSpinnaker.git

## Addons for additional plugins (disabled by default)

* ofxUEye https://github.com/elliotwoods/ofxUeye.git
* ofxOrbbec https://github.com/elliotwoods/ofxOrbbec.git
* ofxZmq https://github.com/elliotwoods/ofxZmq.git

# Changelog

## 0.4

* `ofxDigitalEmulsion` -> `ofxRulr`
* Move to oF 0.9.0 / VS2015
* Introduce plugins for nodes and cameras (e.g. seperate dll files). This helps compatability between machines
* `ofxRulr::Graph::Node` -> `ofxRulr::Nodes::Base`