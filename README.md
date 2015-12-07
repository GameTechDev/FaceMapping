Applying Intel® RealSense™ SDK Face Scans to a 3D Mesh
======================================================

The face mapping sample uses the 3D Scan module to scan the user's face and then map it onto an existing 3D head model. This technique does a "stone face" mapping that is not rigged or currently capable of animating.

Build Instructions
==================
The facescan.sln solution should be built with Visual Studio 2012 or greater. There are 2 build configurations types available, the kind that support scanning faces, and the ones that only supports face mapping of a pre-scanned face. The configurations with the "_NO_RS" prefix don't support face scanning and can be built and run without the RealSense™ SDK or RealSense™ camera

Requirements
============
- Windows 8.1 or Windows 10
- Visual Studio 2012 or higher
- Intel® RealSense™ SDK (R5 release)*
- RealSense™ F200 or SR300 Camera*

* These are only required for scanning new faces. The sample includes a single pre-scanned face.

The Intel® RealSense™ SDK can be download here:
https://software.intel.com/en-us/intel-realsense-sdk/download

For detailed information on this sample, please visit:
https://software.intel.com/en-us/articles/applying-intel-realsense-sdk-face-scans-to-a-3d-mesh

 