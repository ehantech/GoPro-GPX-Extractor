The GPMF structured storage format was originally proposed to store high-frequency periodic sensor data within a video file like an MP4. Action cameras, like that from GoPro, have limited computing resources beyond that needed to store video and audio, so any telemetry storage needed to be lightweight in computation, memory usage and storage bandwidth. While JSON and XML systems where initially considered, the burden on the embedded camera system was too great, so something simpler was needed. While the proposed GPMF structure could be used stand-alone, our intended implementation uses an additional time-indexed track with an MP4, and with an application marker within JPEG images. GPMF share a Key, Length, Value structure (KLV), similar to QuickTime atoms or Interchange File Format (IFF), but the new KLV system is better for describing sensor data.

GPMF -- GoPro Metadata Format or General Purpose Metadata Format -- is a modified Key, Length, Value solution, with a 32-bit aligned payload, that is both compact, full extensible and somewhat human readable in a hex editor. GPMF allows for dependent creation of new FourCC tags, without requiring central registration to define the contents and whether the data is in a nested structure. GPMF is optimized as a time of capture storage format for the collection of sensor data as it happens.


## Quick Start for Developers
in this project we extract some GPMF data from video that captured by **GoPro 8 Hero black**.

we extract **latitude,longitude, altitude, time, speed2d, speed3d**
### requierments:
C++, Qt, ffmpeg, ffpmrob

to use this code, you need to add 3 file into your project.
1. gpsextractor.cpp
2. gpsextractor.h
3. utilsGPX.h


### Sample Code
1. fist include class
   - include <gpsextractor.h>
2. then make a sample of this class
   - GpsExtractor gps;
3. then call method
   - gps.makeGPX("/home/EhanTech/GoPro_GPX/TestVideo.MP4");
