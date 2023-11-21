# Introduction

This plugin includes the nodes for Halo (under Nodes::MirrorPlaneCapture).

# DJI usage

Run the python script here `python/adb_sync.py` to sync files from the DJI Smart Controller to a local folder. Then we can watch that local folder in Rulr to capture photos from it.

Generally we're looking at white-on-black features and we want to reduce the exposure for that purpose (e.g. reduce it down to where the black of the markers is black and the reflection of the drone light isn't blown out).

When calibrating the camera, make sure to also use different camera incline angles (not just facing directly towards the plane of the board, e.g. facing down)
# Marker map creation notes

## Initialisation

* Choose a marker that will be the origin, add this to the `MarkerMap::Markers` node, set its transform manually
	* Have a simple transform for any fixed markers (e.g. if it's on the floor, not (-90, 0, -90) instead just (-90, 0, 0))
* Change settings on Detector node, e.g. turn on `Enclosed markers`
* Put the origin marker into the right space in our heliostat coord system

## Calibration


* Deselect any captures that don't contain the initial marker
* Hit calibrate (now all that see initial marker will be intialised)

# DroneLightInMirror

## Notes

* Wait for servos to settle before taking a capture (e.g. wait 10s after sending any mirror move commands before taking capture)