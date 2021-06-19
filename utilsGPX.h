#ifndef UTILS_H
#define UTILS_H
#include <iostream>
#include <vector>
#include <QString>
#include <QStringList>
#include <map>
#include <QDateTime>
#include <QProcess>
#include <QRegularExpression>
#include <QDebug>

#define TIME_FORMAT "yyyy-MM-ddTHH:mm:ss.zzzZ"

inline std::vector<std::string> _skiplabelsList = {"TIMO", "YAVG", "ISOE", "FACE", "SHUT", "WBAL",
                                        "WRGB", "UNIF", "FCNM", "FWVS", "KBAT", "ATTD",
                                        "GLPI", "VFRH", "BPOS", "ATTR", "SIMU", "ESCS",
                                        "SCPR", "LNED", "CYTS", "CSEN"};

inline std::map<char, std::pair<char,int>> _maptypeList = {{'c', std::make_pair('c',1)},
                                                {'L', std::make_pair('L',4)},
                                                {'s', std::make_pair('h',2)},
                                                {'S', std::make_pair('H',2)},
                                                {'f', std::make_pair('f',4)},
                                                {'U', std::make_pair('c',1)},
                                                {'l', std::make_pair('l',4)},
                                                {'B', std::make_pair('B',1)},
                                                {'J', std::make_pair('Q',8)},
                                                {'?', std::make_pair('?',1)},
                                                {'e', std::make_pair('e',2)},
                                                {'d', std::make_pair('d',8)},
                                                {'p', std::make_pair('p',1)},
                                                {'q', std::make_pair('q',8)},
                                                {'Q', std::make_pair('Q',8)},
                                                {'i', std::make_pair('i',4)},
                                                {'I', std::make_pair('I',4)},
                                                {'h', std::make_pair('h',2)},
                                                {'H', std::make_pair('H',2)}};

template<typename T>
struct AbstractGpsData
{
    T lat;
    T lon;
    T alt;
    T speed;
    T speed3d;
    QString _tostr()
    {
        return QString::number(lat) + " " + QString::number(lon) + " " + QString::number(alt) + " "
               + QString::number(speed) + " " + QString::number(speed3d);
    }
};

template<typename T>
struct AbstractScaleData
{
    T latScale;
    T lonScale;
    T altScale;
    T spee2dScale;
    T speed3dScale;
    QString _tostr()
    {
        return QString::number(latScale) + " " + QString::number(lonScale) + " "
               + QString::number(altScale) + " " + QString::number(spee2dScale) + " "
               + QString::number(speed3dScale);
    }
};

template<typename T>
struct AbstractBaseLblData
{
    T length;
    QString _tostr() { return QString::number(length); }
};

typedef AbstractGpsData<qint32> gpsData32;
typedef AbstractBaseLblData<qint32> baseData32;
typedef  AbstractScaleData<qint32> scaleData32;

struct klvData
{
    std::string fourcc;
    int type;
    int size;
    int repeat;
    int length;
    int padedLength;
    QByteArray rawData;
    QVariant data;
};

struct payLoad
{
    QString time;
    QStringList klvData;
    std::vector<int> scale;
};

struct GpxData
{
    qreal lat;
    qreal lon;
    QDateTime time;
    qreal ele;
    QString comment;
    qreal speed2d;
    qreal speed3d;
};

template<typename T>
T toBigEndian(T u)
{
    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source, destination;

    source.u = u;

    for (size_t k = 0; k < sizeof(T); k++)
        destination.u8[k] = source.u8[sizeof(T) - k - 1];

    return destination.u;
}

#endif // UTILS_H
