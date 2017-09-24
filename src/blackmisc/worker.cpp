/* Copyright (C) 2014
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

#include "blackmisc/threadutils.h"
#include "blackmisc/worker.h"
#include "blackmisc/verify.h"

#include <future>

#ifdef Q_OS_WIN32
#include <windows.h>
#endif

namespace BlackMisc
{
    void CRegularThread::run()
    {
#ifdef Q_OS_WIN32
        m_handle = GetCurrentThread();
        QThread::run();
        m_handle = nullptr;
#else
        QThread::run();
#endif
    }

    CRegularThread::~CRegularThread()
    {
#ifdef Q_OS_WIN32
        auto handle = m_handle.load();
        if (handle)
        {
            auto status = WaitForSingleObject(handle, 0);
            if (isRunning())
            {
                switch (status)
                {
                default:
                case WAIT_FAILED: qWarning() << "Thread" << objectName() << "unspecified error"; break;
                case WAIT_OBJECT_0: qWarning() << "Thread" << objectName() << "unsafely terminated by program shutdown"; break;
                case WAIT_TIMEOUT: break;
                }
            }
        }
#endif
        quit();
        wait();
    }

    CWorker *CWorker::fromTaskImpl(QObject *owner, const QString &name, int typeId, std::function<CVariant()> task)
    {
        auto *worker = new CWorker(task);
        emit worker->aboutToStart();
        worker->setStarted();
        auto *thread = new CRegularThread(owner);

        if (typeId != QMetaType::Void) { worker->m_result = CVariant(typeId, nullptr); }

        QString ownerName = owner->objectName().isEmpty() ? owner->metaObject()->className() : owner->objectName();
        thread->setObjectName(ownerName + ":" + name);
        worker->setObjectName(name);

        worker->moveToThread(thread);
        bool s = QMetaObject::invokeMethod(worker, "ps_runTask");
        Q_ASSERT_X(s, Q_FUNC_INFO, "cannot invoke");
        Q_UNUSED(s);
        thread->start();
        return worker;
    }

    void CWorker::ps_runTask()
    {
        m_result = m_task();

        setFinished();

        auto *ownThread = thread();
        moveToThread(ownThread->thread()); // move worker back to the thread which constructed it, so there is no race on deletion
        QMetaObject::invokeMethod(ownThread, "deleteLater");
        QMetaObject::invokeMethod(this, "deleteLater");
    }

    const CLogCategoryList &CWorkerBase::getLogCategories()
    {
        static const BlackMisc::CLogCategoryList cats { BlackMisc::CLogCategory::worker() };
        return cats;
    }

    void CWorkerBase::waitForFinished() noexcept
    {
        std::promise<void> promise;
        then([ & ] { promise.set_value(); });
        promise.get_future().wait();
    }

    void CWorkerBase::abandon() noexcept
    {
        thread()->requestInterruption();
        quit();
    }

    void CWorkerBase::abandonAndWait() noexcept
    {
        thread()->requestInterruption();
        quitAndWait();
    }

    bool CWorkerBase::isAbandoned() const
    {
        return thread()->isInterruptionRequested();
    }

    CContinuousWorker::CContinuousWorker(QObject *owner, const QString &name) :
        m_owner(owner), m_name(name)
    {
        if (m_name.isEmpty()) { m_name = metaObject()->className(); }
        setObjectName(m_name);
        m_updateTimer.setObjectName(m_name + ":timer");
    }

    void CContinuousWorker::start(QThread::Priority priority)
    {
        BLACK_VERIFY_X(!hasStarted(), Q_FUNC_INFO, "Tried to start a worker that was already started");
        if (hasStarted()) { return; }

        // avoid message "QObject: Cannot create children for a parent that is in a different thread"
        Q_ASSERT_X(CThreadUtils::isCurrentThreadObjectThread(m_owner), Q_FUNC_INFO, "Needs to be started in owner thread");
        emit aboutToStart();
        setStarted();
        auto *thread = new CRegularThread(m_owner);

        Q_ASSERT(m_owner); // must not be null, see (9) https://dev.vatsim-germany.org/issues/402
        if (m_owner)
        {
            const QString ownerName = m_owner->objectName().isEmpty() ? m_owner->metaObject()->className() : m_owner->objectName();
            thread->setObjectName(ownerName + ":" + m_name);
        }

        moveToThread(thread);
        connect(thread, &QThread::started, this, &CContinuousWorker::initialize);
        connect(thread, &QThread::finished, &m_updateTimer, &QTimer::stop);
        connect(thread, &QThread::finished, this, &CContinuousWorker::cleanup);
        connect(thread, &QThread::finished, this, &CContinuousWorker::finish);
        thread->start(priority);
    }

    void CContinuousWorker::quit() noexcept
    {
        Q_ASSERT_X(!CThreadUtils::isApplicationThreadObjectThread(this), Q_FUNC_INFO, "Try to stop main thread");
        setEnabled(false);
        // remark: cannot stop timer here, as I am normally not in the correct thread
        thread()->quit();
    }

    void CContinuousWorker::quitAndWait() noexcept
    {
        Q_ASSERT_X(!CThreadUtils::isApplicationThreadObjectThread(this), Q_FUNC_INFO, "Try to stop main thread");
        Q_ASSERT_X(!CThreadUtils::isCurrentThreadObjectThread(this), Q_FUNC_INFO, "Called by own thread, will deadlock");

        setEnabled(false);
        auto *ownThread = thread();
        quit();
        ownThread->wait();
    }

    void CContinuousWorker::startUpdating(int updateTimeSecs)
    {
        Q_ASSERT_X(hasStarted(), Q_FUNC_INFO, "Worker not yet started");
        if (!CThreadUtils::isCurrentThreadObjectThread(this))
        {
            // shift in correct thread
            if (!this->isFinished())
            {
                QTimer::singleShot(0, this, [this, updateTimeSecs]
                {
                    if (this->isFinished()) { return; }
                    this->startUpdating(updateTimeSecs);
                });
            }
            return;
        }

        // here in correct timer thread
        if (updateTimeSecs < 0)
        {
            setEnabled(false);
            m_updateTimer.stop();
        }
        else
        {
            setEnabled(true);
            m_updateTimer.start(1000 * updateTimeSecs);
        }
    }

    void CContinuousWorker::finish()
    {
        setFinished();

        auto *ownThread = thread();
        moveToThread(ownThread->thread()); // move worker back to the thread which constructed it, so there is no race on deletion
        QMetaObject::invokeMethod(ownThread, "deleteLater");
        QMetaObject::invokeMethod(this, "deleteLater");
    }

}
