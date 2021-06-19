#ifndef GPSEXTRACTOR_H
#define GPSEXTRACTOR_H
#include <utilsGPX.h>
#include <QByteArray>
#include <QFileInfo>

class GpsExtractor
{
public:
    GpsExtractor();
    void makeGPX(const QString &_videoPath);
    void setGpxFilePath(QString gpxFilePath);
    QString saveToFile();

private:
    QString runFFPROBECommand(QString &_cmd, QStringList &_args);
    QByteArray runFFMPEGCommand(const QString &_ffprobeStdOut, const QString &_videoPath);
    bool skip(klvData &_klv);
    void labelTypeUTimeStamp(klvData &_klv);
    int mapType(int &_type);
    void labelGPS5(klvData &_klv);
    void labelBase(klvData &_klv);
    void labelSCAL(klvData &_klv);
    void payloadToSamples(std::vector<payLoad> &_payloadList);
    std::vector<payLoad> makePayloadList(std::vector<klvData> &_klvList);
    void manage(klvData &_klv);
    int pad(int _length);
    QByteArray readRawData(
        int &_type, QByteArray &_ffmpegStdOut, unsigned int &_offset, int &_size, int &_repeat);
    std::vector<klvData> parseStream(QByteArray &_ffmpegStdOut);
    QList<GpxData> _samplesData;
    void append(QPointF coordinate,
                const QDateTime &time,
                double alt,
                double speed2d,
                double speed3d);
    QString _gpxFilePath;
    QString _metaName, _metaDesc, _metaSrc;
    void setMetaName(const QString &_videoPath);
};

#endif // GPSEXTRACTOR_H
