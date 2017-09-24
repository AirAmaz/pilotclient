/* Copyright (C) 2014
 * swift Project Community / Contributors
 *
 * This file is part of swift project. It is subject to the license terms in the LICENSE file found in the top-level
 * directory of this distribution and at http://www.swift-project.org/license.html. No part of swift project,
 * including this file, may be copied, modified, propagated, or distributed except according to the terms
 * contained in the LICENSE file.
 */

//! \file

#ifndef BLACKMISC_WORKER_H
#define BLACKMISC_WORKER_H

#include "blackmisc/blackmiscexport.h"
#include "blackmisc/connectionguard.h"
#include "blackmisc/logcategorylist.h"
#include "blackmisc/invoke.h"
#include "blackmisc/stacktrace.h"
#include "blackmisc/variant.h"

#include <QMetaObject>
#include <QMetaType>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QPointer>
#include <QSharedPointer>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QWeakPointer>
#include <QtGlobal>
#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <atomic>

namespace BlackMisc
{

    template <typename T>
    class CWorkerPointer;

    /*!
     * Starts a single-shot timer which will call a task in the thread of the given object when it times out.
     *
     * Differs from QTimer::singleShot in that this implementation interacts better with QObject::moveToThread.
     */
    //! @{
    template <typename F>
    void singleShot(int msec, QObject *target, F &&task)
    {
        QSharedPointer<QTimer> timer(new QTimer, [](QObject *o) { QMetaObject::invokeMethod(o, "deleteLater"); });
        timer->setSingleShot(true);
        timer->moveToThread(target->thread());
        QObject::connect(timer.data(), &QTimer::timeout, target, [trace = getStackTrace(), task = std::forward<F>(task), timer]() mutable
        {
            static_cast<void>(trace);
            timer.clear();
            task();
        });
        QMetaObject::invokeMethod(timer.data(), "start", Q_ARG(int, msec));
    }
    //! @}

    /*!
     * Just a subclass of QThread whose destructor waits for the thread to finish.
     */
    class BLACKMISC_EXPORT CRegularThread : public QThread
    {
    public:
        //! Constructor
        CRegularThread(QObject *parent = nullptr) : QThread(parent) {}

        //! Destructor
        ~CRegularThread();

    protected:
        //! \copydoc QThread::run
        virtual void run() override;

    private:
        std::atomic<void *> m_handle { nullptr };
    };

    /*!
     * Base class for CWorker and CContinuousWorker.
     */
    class BLACKMISC_EXPORT CWorkerBase : public QObject
    {
        Q_OBJECT

    public:
        //! Log categories
        static const CLogCategoryList &getLogCategories();

        //! Connects to a functor or method which will be called when the task is finished.
        //! \threadsafe
        template <typename T, typename F>
        void then(T *context, F functor)
        {
            Q_ASSERT(context->thread() == QThread::currentThread());
            QMutexLocker lock(&m_finishedMutex);
            connect(this, &CWorkerBase::finished, context, functor);
            if (m_finished) { Private::invokeSlot(functor, context); }
        }

        //! Connects to a functor which will be called when the task is finished.
        //! \threadsafe
        template <typename F>
        void then(F functor)
        {
            QMutexLocker lock(&m_finishedMutex);
            connect(this, &CWorkerBase::finished, functor);
            if (m_finished) { functor(); }
        }

        //! Returns true if the task has finished.
        //! \threadsafe But don't rely on this condition remaining true for any length of time.
        bool isFinished() const
        {
            QMutexLocker lock(&m_finishedMutex);
            return m_finished;
        }

        //! Executes some code (in the caller's thread) if the task has finished.
        //! \threadsafe
        template <typename F>
        void doIfFinished(F functor) const
        {
            QMutexLocker lock(&m_finishedMutex);
            if (m_finished) { functor(); }
        }

        //! Executes some code (in the caller's thread) if the task has not finished.
        //! \threadsafe But the functor will deadlock if it waits for the task to finish.
        template <typename F>
        void doIfNotFinished(F functor) const
        {
            QMutexLocker lock(&m_finishedMutex);
            if (! m_finished) { functor(); }
        }

        //! Executes some code (in the caller's thread) if the task has finished and some different code if it has not finished.
        //! \threadsafe But the elseFunctor will deadlock if it waits for the task to finish.
        template <typename F1, typename F2>
        void doIfFinishedElse(F1 ifFunctor, F2 elseFunctor) const
        {
            QMutexLocker lock(&m_finishedMutex);
            if (m_finished) { ifFunctor(); } else { elseFunctor(); }
        }

        //! Blocks until the task is finished.
        //! \threadsafe Will deadlock if called by the worker thread.
        void waitForFinished() noexcept;

        //! Notify the task that its result is no longer needed, so it can finish early.
        //! \threadsafe
        void abandon() noexcept;

        //! Convenience to call abandon() followed by waitForFinished().
        void abandonAndWait() noexcept;

    signals:
        //! Emitted when the task is about to start.
        void aboutToStart();

        //! Emitted when the task is finished.
        void finished();

    protected:
        //! For the task to check whether it can finish early.
        //! \threadsafe
        bool isAbandoned() const;

        //! True if the worker has started.
        bool hasStarted() const { return m_started; }

        //! Mark the task as started.
        void setStarted() { m_started = true; }

        //! Mark the task as finished.
        void setFinished()
        {
            QMutexLocker lock(&m_finishedMutex);
            m_finished = true;
            emit finished();
        }

    private:
        virtual void quit() noexcept {}
        virtual void quitAndWait() noexcept { waitForFinished(); }

        bool m_started = false;
        bool m_finished = false;
        mutable QMutex m_finishedMutex { QMutex::Recursive };
    };

    /*!
     * Class for doing some arbitrary parcel of work in its own thread.
     *
     * The task is exposed as a function object, so could be a lambda or a hand-written closure.
     * CWorker can not be subclassed, instead it can be extended with rich callable task objects.
     */
    class BLACKMISC_EXPORT CWorker final : public CWorkerBase
    {
        Q_OBJECT

    public:
        /*!
         * Returns a new worker object which lives in a new thread.
         * \note The worker calls its own deleteLater method when finished.
         *       Typically assign it to a QPointer if you want to store it.
         * \param owner Will be the parent of the new thread (the worker has no parent).
         * \param name A name for the task, which will be used to create a name for the thread.
         * \param task A function object which will be run by the worker in its thread.
         */
        template <typename F>
        static CWorker *fromTask(QObject *owner, const QString &name, F &&task)
        {
            int typeId = qMetaTypeId<std::decay_t<decltype(std::forward<F>(task)())>>();
            return fromTaskImpl(owner, name, typeId, [task = std::forward<F>(task)]() { return CVariant::fromResultOf(std::move(task)); });
        }

        //! Connects to a functor to which will be passed the result when the task is finished.
        //! \tparam R The return type of the task.
        //! \threadsafe
        template <typename R, typename F>
        void thenWithResult(F functor)
        {
            Q_ASSERT_X(m_result.canConvert<R>(), Q_FUNC_INFO, "Type in thenWithResult must match return type of task");
            then([this, functor]() { functor(this->result<R>()); });
        }

        //! Connects to a functor or method to which will be passed the result when the task is finished.
        //! \tparam R The return type of the task.
        //! \threadsafe
        template <typename R, typename T, typename F>
        void thenWithResult(T *context, F functor)
        {
            Q_ASSERT_X(m_result.canConvert<R>(), Q_FUNC_INFO, "Type in thenWithResult must match return type of task");
            then(context, [this, context, functor]() { Private::invokeSlot(functor, context, this->result<R>()); });
        }

        //! Returns the result of the task, waiting for it to finish if necessary.
        //! \tparam R The return type of the task.
        //! \threadsafe
        template <typename R>
        R result() { waitForFinished(); Q_ASSERT(m_result.canConvert<R>()); return m_result.value<R>(); }

    private slots:
        //! Called when the worker has been moved into its new thread.
        void ps_runTask();

    private:
        CWorker(std::function<CVariant()> task) : m_task(task) {}
        static CWorker *fromTaskImpl(QObject *owner, const QString &name, int typeId, std::function<CVariant()> task);

        std::function<CVariant()> m_task;
        CVariant m_result;
    };

    /*!
     * Base class for a long-lived worker object which lives in its own thread.
     */
    class BLACKMISC_EXPORT CContinuousWorker : public CWorkerBase
    {
        Q_OBJECT

    public:
        /*!
         * Constructor.
         * \param owner Will be the parent of the new thread (the worker has no parent).
         * \param name A name for the worker, which will be used to create a name for the thread.
         */
        CContinuousWorker(QObject *owner, const QString &name = "") : m_owner(owner), m_name(name) {}

        //! Starts a thread and moves the worker into it.
        void start(QThread::Priority priority = QThread::InheritPriority);

        //! Stops the thread the next time around its event loop.
        //! The thread and the worker will then be deleted.
        //! \threadsafe
        virtual void quit() noexcept final override;

        //! Calls quit() and blocks until the thread is finished.
        //! \threadsafe Will deadlock if called by the worker thread.
        virtual void quitAndWait() noexcept final override;

    protected slots:
        //! Called when the thread is started.
        virtual void initialize() {}

        //! Called when the thread is finished.
        virtual void cleanup() {}

    private slots:
        //! Called after cleanup().
        void ps_finish();

    private:
        template <typename T>
        friend class CWorkerPointer;

        using CWorkerBase::hasStarted;
        using CWorkerBase::setStarted;
        using CWorkerBase::setFinished;

        QObject *m_owner = nullptr;
        QString m_name;
    };

    /*!
     * RAII smart pointer to manage a CContinuousWorker instance.
     *
     * Not required if the worker is immediately started after construction.
     * Before the worker starts, it is owned by the pointer.
     * After the worker starts, becomes a non-owning pointer, as ownership is tied to the lifetime of the thread.
     */
    template <typename T>
    class CWorkerPointer
    {
    public:
        static_assert(std::is_base_of<CContinuousWorker, T>::value, "T must be a CContinuousWorker subclass");

        //! Constructor. Takes ownership.
        explicit CWorkerPointer(T *ptr) : m_weak(ptr)
        {
            if (!ptr || static_cast<const CContinuousWorker *>(ptr)->hasStarted()) { return; }
            m_strong.reset(ptr);
            connect();
        }

        //! Construct a null pointer.
        //! @{
        CWorkerPointer() = default;
        CWorkerPointer(std::nullptr_t) {}
        //! @}

        //! Move constructor.
        CWorkerPointer(CWorkerPointer &&other) : m_strong(std::move(other.m_strong)), m_weak(other.m_weak), m_guard()
        {
            connect();
        }

        //! Move assignment operator.
        CWorkerPointer &operator =(CWorkerPointer &&other)
        {
            m_strong = std::move(other.m_strong);
            m_weak = other.m_weak;
            connect();
            return *this;
        }

        //! Not copyable.
        //! @{
        CWorkerPointer(const CWorkerPointer &) = delete;
        CWorkerPointer &operator =(const CWorkerPointer &) = delete;
        //! @}

        //! Factory method.
        //! Arguments are forwarded to the constructor of T. Strictly more exception-safe than calling the constructor with new.
        template <typename... Ts>
        static CWorkerPointer create(Ts &&... args) { return CWorkerPointer(new T(std::forward<Ts>(args)...)); }

        //! Access the raw pointer.
        //! @{
        T *data() const { return m_weak.data(); }
        T &operator *() const { return *data(); }
        T *operator ->() const { return &*data(); }
        //! @}

        //! True if it points to a valid worker.
        //! @{
        explicit operator bool() const { return m_weak; }
        bool isValid() const { return m_weak; }
        //! @}

        //! True if it owns the worker it points to (i.e. worker has not yet started).
        bool isOwner() const { return m_strong; }

    private:
        void connect()
        {
            if (!m_strong) { return; }
            m_guard = QObject::connect(m_strong.get(), &CWorkerBase::aboutToStart, [this] { m_strong.release(); });
        }

        std::unique_ptr<T> m_strong;
        QPointer<T> m_weak;
        CConnectionGuard m_guard;
    };

}

#endif
