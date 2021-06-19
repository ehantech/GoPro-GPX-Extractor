#include <QCoreApplication>
#include <gpsextractor.h>

int main(int argc, char *argv[])
{
    GpsExtractor gps;
    QString videopath = "/home/EhanTech/GoPro_GPX/TestVideo.MP4";
    gps.makeGPX(videopath);
    return 0;
}
