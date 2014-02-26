#include "blacksound/soundgenerator.h"
#include "blackmisc/filedeleter.h"
#include <QtCore/qendian.h>
#include <math.h>
#include <qmath.h>
#include <qendian.h>
#include <QMultimedia>
#include <QAudioOutput>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QTimer>
#include <QUrl>
#include <QFile>
#include <QDir>

using namespace BlackMisc::Aviation;
using namespace BlackMisc::PhysicalQuantities;
using namespace BlackMisc::Voice;

namespace BlackSound
{
    QDateTime CSoundGenerator::s_selcalStarted  = QDateTime::currentDateTimeUtc();

    CSoundGenerator::CSoundGenerator(const QAudioDeviceInfo &device, const QAudioFormat &format, const QList<Tone> &tones, PlayMode mode, QObject *parent)
        :   QIODevice(parent),
            m_tones(tones), m_position(0), m_playMode(mode), m_endReached(false), m_oneCycleDurationMs(calculateDurationMs(tones)),
            m_device(device), m_audioFormat(format), m_audioOutput(new QAudioOutput(format)),
            m_pushTimer(nullptr), m_pushModeIODevice(nullptr), m_ownThread(nullptr)
    {
        Q_ASSERT(tones.size() > 0);
    }

    CSoundGenerator::CSoundGenerator(const QList<Tone> &tones, PlayMode mode, QObject *parent)
        :   QIODevice(parent),
            m_tones(tones), m_position(0), m_playMode(mode), m_endReached(false), m_oneCycleDurationMs(calculateDurationMs(tones)),
            m_device(QAudioDeviceInfo::defaultOutputDevice()), m_audioFormat(CSoundGenerator::defaultAudioFormat()),
            m_audioOutput(new QAudioOutput(CSoundGenerator::defaultAudioFormat())),
            m_pushTimer(nullptr), m_pushModeIODevice(nullptr), m_ownThread(nullptr)
    {
        Q_ASSERT(tones.size() > 0);
    }

    CSoundGenerator::~CSoundGenerator()
    {
        this->stop(true);
        if (this->m_ownThread) this->m_ownThread->deleteLater();
    }

    void CSoundGenerator::start(int volume, bool pull)
    {
        if (this->m_buffer.isEmpty()) this->generateData();
        this->open(QIODevice::ReadOnly);
        this->m_audioOutput->setVolume(qreal(0.01 * volume));

        if (pull)
        {
            // For an output device, the QAudioOutput class will pull data from the QIODevice
            // (using QIODevice::read()) when more audio data is required.
            this->m_audioOutput->start(this); // pull
        }
        else
        {
            // In push mode, the audio device provides a QIODevice instance that can be
            // written or read to as needed. Typically this results in simpler code but more buffering, which may affect latency.
            if (!this->m_pushTimer)
            {
                this->m_pushTimer = new QTimer(this);
                bool connect = this->connect(this->m_pushTimer, &QTimer::timeout, this, &CSoundGenerator::pushTimerExpired);
                Q_ASSERT(connect);
                this->m_pushTimer->start(20);
            }
            this->m_pushModeIODevice = this->m_audioOutput->start(); // push, IO device not owned
        }
    }

    void CSoundGenerator::startInOwnThread(int volume)
    {
        this->m_ownThread = new QThread(); // deleted by signals, hence no parent
        this->moveToThread(this->m_ownThread);
        // connect(this, &CSoundGenerator::startThread, this, &CSoundGenerator::start);

        connect(this->m_ownThread, &QThread::started, this, [ = ]() { this->start(volume, false); });
        connect(this, &CSoundGenerator::stopping, this->m_ownThread, &QThread::quit);

        // in auto delete mode force deleteLater when thread is finished
        if (this->m_playMode == SingleWithAutomaticDeletion)
            connect(this->m_ownThread, &QThread::finished, this, &CSoundGenerator::deleteLater);

        // start thread and begin processing by calling start via signal startThread
        this->m_ownThread->start();
    }

    void CSoundGenerator::stop(bool destructor)
    {
        // this->m_audioOutput->setVolume(0); // Bug or feature, killing the applicaions volume?
        if (this->isOpen())
        {
            // 1. isOpen avoids redundant signals
            // 2. OK in destructor, see http://stackoverflow.com/a/14024955/356726
            this->close(); // close IO Device
            this->m_audioOutput->stop();
            if (this->m_pushTimer) this->m_pushTimer->stop();
            emit this->stopped();
        }
        this->m_position = 0;
        if (destructor) return;

        // trigger own termination
        if (this->m_playMode == SingleWithAutomaticDeletion)
        {
            emit this->stopping();
            if (!this->m_ownThread) this->deleteLater(); // with own thread, thread signal will call deleteLater
        }
    }

    void CSoundGenerator::pushTimerExpired()
    {
        if (this->m_pushModeIODevice && !this->m_endReached && this->m_audioOutput->state() != QAudio::StoppedState)
        {
            int chunks = this->m_audioOutput->bytesFree() / this->m_audioOutput->periodSize();
            while (chunks)
            {
                // periodSize-> Returns the period size in bytes.
                const qint64 len = this->read(m_buffer.data(), this->m_audioOutput->periodSize());
                if (len)
                    this->m_pushModeIODevice->write(m_buffer.data(), len);
                if (len != this->m_audioOutput->periodSize())
                    break;
                --chunks;
            }
        }
        else
        {
            if (this->m_pushTimer) this->m_pushTimer->stop();
            this->m_pushTimer->disconnect(this);
            if (this->m_playMode == SingleWithAutomaticDeletion) this->stop();
        }
    }

    void CSoundGenerator::generateData()
    {
        Q_ASSERT(this->m_tones.size() > 0);
        const int bytesPerSample = this->m_audioFormat.sampleSize() / 8;
        const int bytesForAllChannels = this->m_audioFormat.channelCount() * bytesPerSample;

        qint64 totalLength = 0;
        foreach(Tone t, this->m_tones)
        {
            totalLength += this->m_audioFormat.sampleRate() * bytesForAllChannels * t.m_durationMs / 1000;
        }

        Q_ASSERT(totalLength % bytesForAllChannels == 0);
        Q_UNUSED(bytesForAllChannels) // suppress warning in release builds

        m_buffer.resize(totalLength);
        unsigned char *bufferPointer = reinterpret_cast<unsigned char *>(m_buffer.data());

        foreach(Tone t, this->m_tones)
        {
            qint64 bytesPerTone = this->m_audioFormat.sampleRate() * bytesForAllChannels * t.m_durationMs / 1000;
            qint64 last0AmplitudeSample = bytesPerTone; // last sample when amplitude was 0
            int sampleIndexPerTone = 0;
            while (bytesPerTone)
            {
                // http://hyperphysics.phy-astr.gsu.edu/hbase/audio/sumdif.html
                // http://math.stackexchange.com/questions/164369/how-do-you-calculate-the-frequency-perceived-by-humans-of-two-sinusoidal-waves-a
                const double pseudoTime = double(sampleIndexPerTone % this->m_audioFormat.sampleRate()) / this->m_audioFormat.sampleRate();
                double amplitude = 0.0; // amplitude -1 -> +1 , 0 is silence
                if (t.m_frequencyHz > 10)
                {
                    // the combination of two frequencies actually would have 2*amplitude,
                    // but I have to normalize with amplitude -1 -> +1

                    amplitude = t.m_secondaryFrequencyHz == 0 ?
                                qSin(2 * M_PI * t.m_frequencyHz * pseudoTime) :
                                qSin(M_PI * (t.m_frequencyHz + t.m_secondaryFrequencyHz) * pseudoTime) *
                                qCos(M_PI * (t.m_frequencyHz - t.m_secondaryFrequencyHz) * pseudoTime);
                }

                // avoid overflow
                Q_ASSERT(amplitude <= 1.0 && amplitude >= -1.0);
                if (amplitude < -1.0)
                    amplitude = -1.0;
                else if (amplitude > 1.0)
                    amplitude = 1.0;
                else if (qAbs(amplitude) < double(1.0 / 65535))
                {
                    amplitude = 0;
                    last0AmplitudeSample = bytesPerTone;
                }

                // generate this for all channels, usually 1 channel
                for (int i = 0; i < this->m_audioFormat.channelCount(); ++i)
                {
                    this->writeAmplitudeToBuffer(amplitude, bufferPointer);
                    bufferPointer += bytesPerSample;
                    bytesPerTone -= bytesPerSample;
                }
                ++sampleIndexPerTone;
            }

            // fixes the range from the last 0 pass through
            if (last0AmplitudeSample > 0)
            {
                bufferPointer -= last0AmplitudeSample;
                while (last0AmplitudeSample)
                {
                    double amplitude = 0.0; // amplitude -1 -> +1 , 0 is silence

                    // generate this for all channels, usually 1 channel
                    for (int i = 0; i < this->m_audioFormat.channelCount(); ++i)
                    {
                        this->writeAmplitudeToBuffer(amplitude, bufferPointer);
                        bufferPointer += bytesPerSample;
                        last0AmplitudeSample -= bytesPerSample;
                    }
                }
            }
        }
    }

    void CSoundGenerator::writeAmplitudeToBuffer(const double amplitude, unsigned char *bufferPointer)
    {
        if (this->m_audioFormat.sampleSize() == 8 && this->m_audioFormat.sampleType() == QAudioFormat::UnSignedInt)
        {
            const quint8 value = static_cast<quint8>((1.0 + amplitude) / 2 * 255);
            *reinterpret_cast<quint8 *>(bufferPointer) = value;
        }
        else if (this->m_audioFormat.sampleSize() == 8 && this->m_audioFormat.sampleType() == QAudioFormat::SignedInt)
        {
            const qint8 value = static_cast<qint8>(amplitude * 127);
            *reinterpret_cast<quint8 *>(bufferPointer) = value;
        }
        else if (this->m_audioFormat.sampleSize() == 16 && this->m_audioFormat.sampleType() == QAudioFormat::UnSignedInt)
        {
            quint16 value = static_cast<quint16>((1.0 + amplitude) / 2 * 65535);
            if (this->m_audioFormat.byteOrder() == QAudioFormat::LittleEndian)
                qToLittleEndian<quint16>(value, bufferPointer);
            else
                qToBigEndian<quint16>(value, bufferPointer);
        }
        else if (this->m_audioFormat.sampleSize() == 16 && this->m_audioFormat.sampleType() == QAudioFormat::SignedInt)
        {
            qint16 value = static_cast<qint16>(amplitude * 32767);
            if (this->m_audioFormat.byteOrder() == QAudioFormat::LittleEndian)
                qToLittleEndian<qint16>(value, bufferPointer);
            else
                qToBigEndian<qint16>(value, bufferPointer);
        }
    }

    bool CSoundGenerator::saveToWavFile(const QString &fileName) const
    {
        QFile file(fileName);
        bool success = file.open(QIODevice::WriteOnly);
        if (!success) return false;

        CombinedHeader header;
        size_t headerLength = sizeof(CombinedHeader);
        memset(&header, 0, headerLength);

        // RIFF header
        if (m_audioFormat.byteOrder() == QAudioFormat::LittleEndian)
            memcpy(&header.riff.descriptor.id[0], "RIFF", 4);
        else
            memcpy(&header.riff.descriptor.id[0], "RIFX", 4);

        qToLittleEndian<quint32>(quint32(m_buffer.size() + headerLength - 8),
                                 reinterpret_cast<unsigned char *>(&header.riff.descriptor.size));
        memcpy(&header.riff.type[0], "WAVE", 4);

        // WAVE header
        memcpy(&header.wave.descriptor.id[0], "fmt ", 4);
        qToLittleEndian<quint32>(quint32(16),
                                 reinterpret_cast<unsigned char *>(&header.wave.descriptor.size));
        qToLittleEndian<quint16>(quint16(1),
                                 reinterpret_cast<unsigned char *>(&header.wave.audioFormat));
        qToLittleEndian<quint16>(quint16(m_audioFormat.channelCount()),
                                 reinterpret_cast<unsigned char *>(&header.wave.numChannels));
        qToLittleEndian<quint32>(quint32(m_audioFormat.sampleRate()),
                                 reinterpret_cast<unsigned char *>(&header.wave.sampleRate));
        qToLittleEndian<quint32>(quint32(m_audioFormat.sampleRate() * m_audioFormat.channelCount() * m_audioFormat.sampleSize() / 8),
                                 reinterpret_cast<unsigned char *>(&header.wave.byteRate));
        qToLittleEndian<quint16>(quint16(m_audioFormat.channelCount() * m_audioFormat.sampleSize() / 8),
                                 reinterpret_cast<unsigned char *>(&header.wave.blockAlign));
        qToLittleEndian<quint16>(quint16(m_audioFormat.sampleSize()),
                                 reinterpret_cast<unsigned char *>(&header.wave.bitsPerSample));

        // DATA header
        memcpy(&header.data.descriptor.id[0], "data", 4);
        qToLittleEndian<quint32>(quint32(this->m_buffer.size()),
                                 reinterpret_cast<unsigned char *>(&header.data.descriptor.size));

        success = file.write(reinterpret_cast<const char *>(&header), headerLength) == headerLength;
        success = success && file.write(this->m_buffer) == this->m_buffer.size();
        file.close();
        return success;
    }

    qint64 CSoundGenerator::calculateDurationMs(const QList<CSoundGenerator::Tone> &tones)
    {
        if (tones.isEmpty()) return 0;
        qint64 d = 0;
        foreach(Tone t, tones)
        {
            d += t.m_durationMs;
        }
        return d;
    }

    qint64 CSoundGenerator::readData(char *data, qint64 len)
    {
        if (len < 1) return 0;
        if (this->m_endReached)
        {
            this->stop(); // all data read, we can stop output
            return 0;
        }
        if (!this->isOpen()) return 0;
        qint64 total = 0; // toal is used for the overflow when starting new wave again
        while (len - total > 0)
        {
            const qint64 chunkSize = qMin((m_buffer.size() - m_position), (len - total));
            memcpy(data + total, m_buffer.constData() + m_position, chunkSize);
            this->m_position = (m_position + chunkSize) % m_buffer.size();
            total += chunkSize;
            if (m_position == 0 &&
                    (m_playMode == Single || m_playMode == SingleWithAutomaticDeletion))
            {
                this->m_endReached = true;
                break;
            }
        }
        return total;
    }

    qint64 CSoundGenerator::writeData(const char *data, qint64 len)
    {
        Q_UNUSED(data);
        Q_UNUSED(len);
        return 0;
    }

    qint64 CSoundGenerator::bytesAvailable() const
    {
        return m_buffer.size() + QIODevice::bytesAvailable();
    }

    QAudioFormat CSoundGenerator::defaultAudioFormat()
    {
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(1);
        format.setSampleSize(16); // 8 or 16 works
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);
        return format;
    }

    /*
     * BlackMisc to Qt audio device
     */
    QAudioDeviceInfo CSoundGenerator::findClosestOutputDevice(const BlackMisc::Voice::CAudioDevice &audioDevice)
    {
        Q_ASSERT(audioDevice.getType() == CAudioDevice::OutputDevice);
        const QString lookFor = audioDevice.getName().toLower();
        QAudioDeviceInfo qtDevice = QAudioDeviceInfo::defaultOutputDevice();
        if (lookFor.startsWith("default")) return qtDevice;
        int score = 0;
        foreach(QAudioDeviceInfo qd, QAudioDeviceInfo::availableDevices(QAudio::AudioOutput))
        {
            const QString cn = qd.deviceName().toLower();
            if (lookFor == cn) return qd; // exact match
            if (cn.length() < lookFor.length())
            {
                if (lookFor.contains(cn) && cn.length() > score)
                {
                    qtDevice = qd;
                    score = cn.length();
                }
            }
            else
            {
                if (cn.contains(lookFor) && lookFor.length() > score)
                {
                    qtDevice = qd;
                    score = lookFor.length();
                }
            }
        }
        return qtDevice;

    }

    CSoundGenerator *CSoundGenerator::playSignal(qint32 volume, const QList<CSoundGenerator::Tone> &tones, QAudioDeviceInfo device)
    {
        CSoundGenerator *generator = new CSoundGenerator(device, CSoundGenerator::defaultAudioFormat(), tones, CSoundGenerator::SingleWithAutomaticDeletion);
        if (tones.isEmpty()) return generator; // that was easy
        if (volume < 1) return generator;
        if (generator->singleCyleDurationMs() < 10) return generator; // unable to hear

        // play, and maybe clean up when done
        generator->start(volume);
        return generator;
    }

    CSoundGenerator *CSoundGenerator::playSignalInBackground(qint32 volume, const QList<CSoundGenerator::Tone> &tones, QAudioDeviceInfo device)
    {
        CSoundGenerator *generator = new CSoundGenerator(device, CSoundGenerator::defaultAudioFormat(), tones, CSoundGenerator::SingleWithAutomaticDeletion);
        if (tones.isEmpty()) return generator; // that was easy
        if (volume < 1) return generator;
        if (generator->singleCyleDurationMs() < 10) return generator; // unable to hear

        // play, and maybe clean up when done
        generator->startInOwnThread(volume);
        return generator;
    }

    void CSoundGenerator::playSignalRecorded(qint32 volume, const QList<CSoundGenerator::Tone> &tones, QAudioDeviceInfo device)
    {
        if (tones.isEmpty()) return; // that was easy
        if (volume < 1) return;

        CSoundGenerator *generator = new CSoundGenerator(device, CSoundGenerator::defaultAudioFormat(), tones, CSoundGenerator::SingleWithAutomaticDeletion);
        if (generator->singleCyleDurationMs() > 10)
        {
            // play, and maybe clean up when done
            QString fileName = QString("blacksound").append(QString::number(QDateTime::currentMSecsSinceEpoch())).append(".wav");
            fileName = QDir::temp().filePath(fileName);
            generator->generateData();
            generator->saveToWavFile(fileName);
            CSoundGenerator::playFile(volume, fileName, true);
        }
        generator->deleteLater();
    }

    void CSoundGenerator::playSelcal(qint32 volume, const BlackMisc::Aviation::CSelcal &selcal, QAudioDeviceInfo device)
    {
        QList<CSoundGenerator::Tone> tones;
        if (selcal.isValid())
        {
            QList<CFrequency> frequencies = selcal.getFrequencies();
            Q_ASSERT(frequencies.size() == 4);
            const BlackMisc::PhysicalQuantities::CTime oneSec(1000.0, BlackMisc::PhysicalQuantities::CTimeUnit::ms());
            Tone t1(frequencies.at(0), frequencies.at(1), oneSec);
            Tone t2(CFrequency(), oneSec / 5.0);
            Tone t3(frequencies.at(2), frequencies.at(3), oneSec);
            tones << t1 << t2 << t3;
        }
        CSoundGenerator::playSignalInBackground(volume, tones, device);
        // CSoundGenerator::playSignalRecorded(volume, tones, device);
    }

    void CSoundGenerator::playSelcal(qint32 volume, const CSelcal &selcal, const CAudioDevice &audioDevice)
    {
        if (CSoundGenerator::s_selcalStarted.msecsTo(QDateTime::currentDateTimeUtc()) < 2500) return; // simple check not to play 2 SELCAL at the same time
        CSoundGenerator::s_selcalStarted = QDateTime::currentDateTimeUtc();
        CSoundGenerator::playSelcal(volume, selcal, CSoundGenerator::findClosestOutputDevice(audioDevice));
    }

    void CSoundGenerator::playNotificationSound(qint32 volume, CSoundGenerator::Notification notification)
    {
        QMediaPlayer *mediaPlayer = CSoundGenerator::mediaPlayer();
        if (mediaPlayer->state() == QMediaPlayer::PlayingState) return;
        QMediaPlaylist *playlist = mediaPlayer->playlist();
        if (!playlist || playlist->isEmpty())
        {
            // order here is crucial, needs to be the same as in CSoundGenerator::Notification
            if (!playlist) playlist = new QMediaPlaylist(mediaPlayer);
            bool success = true;
            success = playlist->addMedia(QUrl::fromLocalFile(QCoreApplication::applicationDirPath().append("/sounds/error.wav"))) && success;
            success = playlist->addMedia(QUrl::fromLocalFile(QCoreApplication::applicationDirPath().append("/sounds/login.wav"))) && success;
            success = playlist->addMedia(QUrl::fromLocalFile(QCoreApplication::applicationDirPath().append("/sounds/logoff.wav"))) && success;
            success = playlist->addMedia(QUrl::fromLocalFile(QCoreApplication::applicationDirPath().append("/sounds/privatemessage.wav"))) && success;
            Q_ASSERT(success);
            playlist->setPlaybackMode(QMediaPlaylist::CurrentItemOnce);
            mediaPlayer->setPlaylist(playlist);
        }
        int index = static_cast<int>(notification);
        playlist->setCurrentIndex(index);
        mediaPlayer->setVolume(volume); // 0-100
        mediaPlayer->play();
    }

    void CSoundGenerator::playFile(qint32 volume, const QString &file, bool removeFileAfterPlaying)
    {
        if (!QFile::exists(file)) return;
        QMediaPlayer *mediaPlayer = CSoundGenerator::mediaPlayer();
        QMediaResource mediaResource(QUrl(file), "audio");
        QMediaContent media(mediaResource);
        mediaPlayer->setMedia(media);
        mediaPlayer->setVolume(volume); // 0-100
        mediaPlayer->play();
        // I cannot delete the file here, only after it has been played
        if (removeFileAfterPlaying) BlackMisc::CFileDeleter::addFileForDeletion(file);
    }
} // namespace
