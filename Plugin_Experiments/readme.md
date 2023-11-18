# Introduction

This plugin includes the nodes for Halo (under Nodes::MirrorPlaneCapture).

# DJI usage

Run the python script here `python/adb_sync.py` to sync files from the DJI Smart Controller to a local folder. Then we can watch that local folder in Rulr to capture photos from it.

# Marker map creation notes

## Initialisation

* Choose a marker that will be the origin, add this to the `MarkerMap::Markers` node, set its transform manually
	* Have a simple transform for any fixed markers (e.g. if it's on the floor, not (-90, 0, -90) instead just (-90, 0, 0))
* Change settings on Detector node, e.g. turn on `Enclosed markers`
