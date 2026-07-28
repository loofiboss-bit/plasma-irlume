// SPDX-License-Identifier: GPL-3.0-or-later

#include "camerapreviewsession.h"

#include <QGuiApplication>
#include <QTextStream>
#include <QTimer>

class HardwareCheck final : public QObject
{
    Q_OBJECT

  public:
    explicit HardwareCheck(QCoreApplication *application)
        : m_application(application),
          m_session(QCoreApplication::applicationDirPath() + QStringLiteral("/plasma-irlume-camera-preview-worker"),
                    this)
    {
        connect(&m_session, &CameraPreviewSession::stateChanged, this, &HardwareCheck::advance);
        connect(&m_session, &CameraPreviewSession::frameChanged, this,
                [this]()
                {
                    if (!m_session.frameAvailable() || m_stopping)
                        return;
                    QTextStream(stdout) << "device=" << (m_index + 1) << " spectrum=" << m_session.spectrum()
                                        << " preview=pass\n";
                    m_stopping = true;
                    m_session.stopPreview();
                });
        QTimer::singleShot(120000, this,
                           [this]()
                           {
                               QTextStream(stderr) << "result=fail error=hardware-check-timeout\n";
                               finish(1);
                           });
    }

    void start()
    {
        m_session.refreshDevices();
    }

  private:
    void advance()
    {
        if (m_session.state() == CameraPreviewSession::State::Failed)
        {
            QTextStream(stderr) << "result=fail error=" << m_session.errorCode() << '\n';
            finish(1);
            return;
        }
        if (m_session.state() != CameraPreviewSession::State::Ready)
            return;
        if (!m_discoveryReported)
        {
            m_discoveryReported = true;
            QTextStream(stdout) << "devices=" << m_session.deviceCount()
                                << " rgb=" << m_session.deviceCountForSpectrum(QStringLiteral("rgb"))
                                << " ir=" << m_session.deviceCountForSpectrum(QStringLiteral("ir"))
                                << " unknown=" << m_session.deviceCountForSpectrum(QStringLiteral("unknown")) << '\n';
        }
        else if (m_stopping)
        {
            QTextStream(stdout) << "device=" << (m_index + 1) << " released=pass\n";
            ++m_index;
            m_stopping = false;
        }

        if (m_index >= m_session.deviceCount())
        {
            QTextStream(stdout) << "result=pass\n";
            finish(0);
            return;
        }
        m_session.setSelectedDeviceIndex(m_index);
        m_session.startPreview();
    }

    void finish(int exitCode)
    {
        if (m_finishing)
            return;
        m_finishing = true;
        QCoreApplication *application = m_application;
        deleteLater();
        QTimer::singleShot(200, application, [application, exitCode]() { application->exit(exitCode); });
    }

    QCoreApplication *m_application = nullptr;
    CameraPreviewSession m_session;
    int m_index = 0;
    bool m_discoveryReported = false;
    bool m_stopping = false;
    bool m_finishing = false;
};

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    auto *check = new HardwareCheck(&application);
    QTimer::singleShot(0, check, &HardwareCheck::start);
    return application.exec();
}

#include "camera_hardware_check.moc"
