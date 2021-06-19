#include "gpsextractor.h"
#include <QPointF>
#include <QXmlStreamWriter>

GpsExtractor::GpsExtractor() {}

void GpsExtractor::makeGPX(const QString &_videoPath)
{
    QString _command = "ffprobe";
    QStringList _args;
    _args << _videoPath;
    QString _ffprobeStdOut = runFFPROBECommand(_command, _args);
    QByteArray _ffmpegStdOut = runFFMPEGCommand(_ffprobeStdOut, _videoPath);
    std::vector<klvData> _klvList = parseStream(_ffmpegStdOut);
    std::vector<payLoad> _payloadList = makePayloadList(_klvList);
    payloadToSamples(_payloadList);
    QString _tmpGpxPath = _videoPath;
    _tmpGpxPath = _tmpGpxPath.mid(0, _tmpGpxPath.lastIndexOf(".")) + ".gpx";
    setGpxFilePath(_tmpGpxPath);
    setMetaName(_videoPath);
    saveToFile();
}

void GpsExtractor::setGpxFilePath(QString gpxFilePath)
{
    if (_gpxFilePath == gpxFilePath)
        return;
    _gpxFilePath = gpxFilePath;
}

QString GpsExtractor::saveToFile()
{
    QFile f(_gpxFilePath);
    f.open(QIODevice::WriteOnly);
    QXmlStreamWriter w(&f);

    w.writeProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    w.setAutoFormattingIndent(4);
    w.setAutoFormatting(true);

    w.writeStartElement("gpx");
    w.writeAttribute("xmlns", "http://www.topografix.com/GPX/1/1");
    w.writeAttribute("version", "1.1");
    w.writeAttribute("creator", "https://github.com/juanirache/gopro-telemetry");

    w.writeStartElement("trk");
    w.writeTextElement("name", _metaName);
    w.writeTextElement("desc", _metaDesc);
    w.writeTextElement("src", _metaSrc);

    w.writeStartElement("trkseg");

    for (auto &pt : _samplesData) {
        w.writeStartElement("trkpt");
        w.writeAttribute("lat", QString::number(pt.lat, 'f', 7));
        w.writeAttribute("lon", QString::number(pt.lon, 'f', 7));

        w.writeTextElement("ele", QString::number(pt.ele));
        w.writeTextElement("time", pt.time.toString(TIME_FORMAT));
        w.writeTextElement("cmt", pt.comment);

        w.writeEndElement();
    }
    w.writeEndDocument();
    f.close();
    qDebug() << "Number of all points: " << _samplesData.size();
    qDebug() << "GPX data is saved in:" << f.fileName();

    return f.fileName();
}

QString GpsExtractor::runFFPROBECommand(QString &_cmd, QStringList &_args)
{
    QProcess _qProcess;
    _qProcess.start(_cmd, _args, QIODevice::ReadOnly);
    _qProcess.waitForFinished(-1);
    QString _StdOut = _qProcess.readAllStandardOutput();
    QString _StdError = _qProcess.readAllStandardError();
    if (_StdOut.length() == 0) {
        _StdOut = _StdError;
    }
    return _StdOut;
}

QByteArray GpsExtractor::runFFMPEGCommand(const QString &_ffprobeStdOut, const QString &_videoPath)
{
    QProcess _qProcess;
    QRegularExpression _re("Stream #\\d:(\\d)\\(.+\\): Data: \\w+ \\(gpmd");
    QRegularExpressionMatch _match = _re.match(_ffprobeStdOut);

    if (_match.hasMatch()) {
        QString _lineinfo = _match.captured(0);
        QString _track_number = _match.captured(1);

        QString _ffmpegCommand = "ffmpeg";
        QStringList _ffmpegArgs;
        _ffmpegArgs << "-y"
                    << "-i" << _videoPath << "-codec"
                    << "copy"
                    << "-map"
                    << "0:" + _track_number << "-f"
                    << "rawvideo"
                    << "-";
        _qProcess.start(_ffmpegCommand, _ffmpegArgs);
        _qProcess.waitForFinished(-1);
    }
    return _qProcess.readAll();
}

bool GpsExtractor::skip(klvData &_klv)
{
    if (std::find(_skiplabelsList.begin(), _skiplabelsList.end(), _klv.fourcc)
        != _skiplabelsList.end()) {
        return true;
    }
    return false;
}

void GpsExtractor::labelTypeUTimeStamp(klvData &_klv)
{
    QDateTime _dateTime2 = QDateTime::fromString(QString::fromLocal8Bit(_klv.rawData),
                                                 "yyMMddHHmmss.zzz")
                               .addYears(100);
    _klv.data.setValue(_dateTime2.toString("yyyy-MM-ddTHH:mm:ss.zzzZ"));
}

int GpsExtractor::mapType(int &_type)
{
    char _ctype = _type;
    if (_maptypeList.find(_ctype) != _maptypeList.end()) {
        return _maptypeList[_ctype].second;
    }
    return _maptypeList[_ctype].second;
}

void GpsExtractor::labelGPS5(klvData &_klv)
{
    gpsData32 _gps5Data;
    if (_klv.rawData.size() == 0) {
        _gps5Data.lat = 0;
        _gps5Data.lon = 0;
        _gps5Data.alt = 0;
        _gps5Data.speed = 0;
        _gps5Data.speed3d = 0;

    } else {
        QStringList _tmpStrList;
        for (int i = 0; i < _klv.repeat; i++) {
            int _stype = mapType(_klv.type);
            QByteArray _gps = _klv.rawData.mid(i * 20, 20).mid(0, 5 * _stype);
            auto _rawData = _gps.data();
            memcpy(&_gps5Data, _rawData, sizeof(gpsData32));
            _gps5Data.lat = toBigEndian<qint32>(_gps5Data.lat);
            _gps5Data.lon = toBigEndian<qint32>(_gps5Data.lon);
            _gps5Data.alt = toBigEndian<qint32>(_gps5Data.alt);
            _gps5Data.speed = toBigEndian<qint32>(_gps5Data.speed);
            _gps5Data.speed3d = toBigEndian<qint32>(_gps5Data.speed3d);
            _tmpStrList.push_back(_gps5Data._tostr());
        }
        _klv.data.setValue(_tmpStrList);
    }
}

void GpsExtractor::labelBase(klvData &_klv)
{
    baseData32 _baseData;
    if (_klv.rawData.size() == 0) {
        return;
    } else {
        int _stype = mapType(_klv.type);
        QByteArray _gps = _klv.rawData.mid(0, _stype);
        auto _rawData = _gps.data();
        memcpy(&_baseData, _rawData, sizeof(baseData32));
        if (_stype == 2) {
            _baseData.length = toBigEndian<qint16>(_baseData.length);
        } else {
            _baseData.length = toBigEndian<qint32>(_baseData.length);
        }
        _klv.data.setValue(_baseData._tostr());
    }
}

void GpsExtractor::labelSCAL(klvData &_klv)
{
    scaleData32 _scaleData;
    if (_klv.repeat == 1) {
        labelBase(_klv);
    } else {
        int _stype = mapType(_klv.type);
        QByteArray _gps = _klv.rawData.mid(0, _stype * _klv.repeat);
        auto _rawData = _gps.data();
        memcpy(&_scaleData, _rawData, sizeof(gpsData32));
        _scaleData.latScale = toBigEndian<qint32>(_scaleData.latScale);
        _scaleData.lonScale = toBigEndian<qint32>(_scaleData.lonScale);
        _scaleData.altScale = toBigEndian<qint32>(_scaleData.altScale);
        _scaleData.spee2dScale = toBigEndian<qint32>(_scaleData.spee2dScale);
        _scaleData.speed3dScale = toBigEndian<qint32>(_scaleData.speed3dScale);
        _klv.data.setValue(_scaleData._tostr());
    }
}

void GpsExtractor::payloadToSamples(std::vector<payLoad> &_payloadList)
{
    int _timeStamp = 0;
    for (size_t i = 0; i < _payloadList.size() - 1; i++) {
        QDateTime _thisTime = QDateTime::fromString(_payloadList[i].time,
                                                    "yyyy-MM-ddTHH:mm:ss.zzzZ");
        _thisTime.setTimeSpec(Qt::UTC);
        QDateTime _nextTime = QDateTime::fromString(_payloadList[i + 1].time,
                                                    "yyyy-MM-ddTHH:mm:ss.zzzZ");
        _nextTime.setTimeSpec(Qt::UTC);
        int _diff = _nextTime.toMSecsSinceEpoch() - _thisTime.toMSecsSinceEpoch();

        QStringList _thisData = _payloadList[i].klvData;
        _timeStamp = static_cast<int>(_diff) / _thisData.size();

        for (size_t j = 0; j < size_t(_thisData.size()); j++) {
            QDateTime _newTime = _thisTime.addMSecs(_timeStamp * j);
            QString _elements = _thisData[j];
            append(QPointF{_elements.split(" ")[0].toDouble() / _payloadList[i].scale[0],
                           _elements.split(" ")[1].toDouble() / _payloadList[i].scale[1]},
                   _newTime,
                   _elements.split(" ")[2].toDouble() / _payloadList[i].scale[2],
                   _elements.split(" ")[3].toDouble() / _payloadList[i].scale[3],
                   _elements.split(" ")[4].toDouble() / _payloadList[i].scale[4]);
        }
    }

    QDateTime _thisTime = QDateTime::fromString(_payloadList.back().time,
                                                "yyyy-MM-ddTHH:mm:ss.zzzZ");
    _thisTime.setTimeSpec(Qt::UTC);
    QStringList _thisData = _payloadList.back().klvData;

    for (size_t j = 0; j < size_t(_thisData.size()); j++) {
        QDateTime _newTime = _thisTime.addMSecs(_timeStamp * j);
        QString _elements = _thisData[j];
        append(QPointF{_elements.split(" ")[0].toDouble() / _payloadList.back().scale[0],
                       _elements.split(" ")[1].toDouble() / _payloadList.back().scale[1]},
               _newTime,
               _elements.split(" ")[2].toDouble() / _payloadList.back().scale[2],
               _elements.split(" ")[3].toDouble() / _payloadList.back().scale[3],
               _elements.split(" ")[4].toDouble() / _payloadList.back().scale[4]);
    }
}

std::vector<payLoad> GpsExtractor::makePayloadList(std::vector<klvData> &_klvList)
{
    std::vector<int> _scaleVec;
    QString _time;
    std::vector<payLoad> _payloadList;
    for (klvData &_klv : _klvList) {
        if (_klv.fourcc == "SCAL") {
            QString _str = _klv.data.toString();
            if (_str.split(" ").size() > 1) {
                int _latScale = _str.split(" ")[0].toDouble();
                int _lonScale = _str.split(" ")[1].toDouble();
                int _altScale = _str.split(" ")[2].toDouble();
                int _speed2dScale = _str.split(" ")[3].toDouble();
                int _speed3dScale = _str.split(" ")[4].toDouble();
                _scaleVec = {_latScale, _lonScale, _altScale, _speed2dScale, _speed3dScale};
            }
        }

        else if (_klv.fourcc == "GPSU") {
            _time = _klv.data.toString();
        }

        else if (_klv.fourcc == "GPSF") {
        }

        else if (_klv.fourcc == "GPS5") {
            payLoad _pl;
            _pl.klvData = _klv.data.toStringList();
            _pl.scale = _scaleVec;
            _pl.time = _time;
            _payloadList.push_back(_pl);
        }

        else if (_klv.fourcc == "SYST") {
        }

        else if (_klv.fourcc == "GPRI") {
        }
    }
    return _payloadList;
}

void GpsExtractor::manage(klvData &_klv)
{
    if (_klv.fourcc == "GPSU") {
        labelTypeUTimeStamp(_klv);
    }

    else if (_klv.fourcc == "GPS5") {
        labelGPS5(_klv);
    }

    else if (_klv.fourcc == "GPSF") {
        labelBase(_klv);
    } else if (_klv.fourcc == "SCAL") {
        labelSCAL(_klv);
    }
}

int GpsExtractor::pad(int _length)
{
    int _base = 4;
    while (_length % _base != 0) {
        _length++;
    }
    return _length;
}

QByteArray GpsExtractor::readRawData(
    int &_type, QByteArray &_ffmpegStdOut, unsigned int &_offset, int &_size, int &_repeat)
{
    QByteArray _result;
    if (_type == 0) {
        return _result;
    }
    unsigned int _numBytes = pad(_size * _repeat);
    if (_numBytes == 0) {
        return _result;
    } else {
        _result = _ffmpegStdOut.mid(_offset + 8, _numBytes);
        return _result;
    }
}

std::vector<klvData> GpsExtractor::parseStream(QByteArray &_ffmpegStdOut)
{
    std::vector<klvData> _klvlist;
    unsigned int _offset = 0;
    unsigned int _packSize = 8;

    while (_offset < uint(_ffmpegStdOut.size())) {
        klvData _klv;
        std::string _fourcc = "";
        for (unsigned int i = _offset; i < _offset + 4; i++) {
            char _c = _ffmpegStdOut[i];
            _fourcc += _c;
        }

        int _type = _ffmpegStdOut[_offset + 4];
        int _size = _ffmpegStdOut[_offset + 5];
        int _repeat = _ffmpegStdOut[_offset + 6] * 256 + _ffmpegStdOut[_offset + 7];
        if (_repeat < 0) {
            _repeat += 256;
        }
        int _length = _size * _repeat;

        int _padedlength = pad(_length);

        _klv.rawData = readRawData(_type, _ffmpegStdOut, _offset, _size, _repeat);
        _klv.fourcc = _fourcc;
        _klv.length = _length;
        _klv.padedLength = _padedlength;
        _klv.type = _type;
        _klv.size = _size;
        _klv.repeat = _repeat;
        manage(_klv);
        if (!skip(_klv)) {
            _klvlist.push_back(_klv);
        }

        _offset += _packSize;
        if (_type != 0) {
            _offset += _padedlength;
        }
    }
    return _klvlist;
}

void GpsExtractor::append(QPointF coordinate,
                          const QDateTime &time,
                          double alt,
                          double speed2d,
                          double speed3d
                          )
{
    GpxData d;
    d.lat  = coordinate.x();
    d.lon  = coordinate.y();
    d.ele = alt;
    d.speed2d = speed2d;
    d.speed3d = speed3d;
    d.comment = "altitude system: MSLV; 2dSpeed: " + QString::number(d.speed2d)
                + "; 3dSpeed: " + QString::number(d.speed3d);
    d.time = time;
    _samplesData.append(d);
}

void GpsExtractor::setMetaName(const QString &_videoPath)
{
    QFileInfo fi(_videoPath);
    _metaName = fi.fileName();
}
