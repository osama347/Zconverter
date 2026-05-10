#pragma once
#include <QObject>
#include <QList>
#include <memory>
#include <atomic>
#include "ConversionJob.h"
#include "IConverter.h"
#include "ThreadPool.h"

namespace zc {

    class ConversionEngine : public QObject
    {
        Q_OBJECT

    public:
        explicit ConversionEngine(QObject* parent = nullptr);
        ~ConversionEngine() override;

        // Add a converter plugin
        void registerConverter(std::shared_ptr<IConverter> converter);

        // Find which converter handles this job
        IConverter* findConverter(const QString& fromExt,
            const QString& toExt) const;

        // Submit a job to the queue
        void submitJob(ConversionJob job);

        // Cancel all pending jobs
        void cancelAll();

        bool isRunning() const;

    signals:
        // Emitted on worker thread — connect with Qt::QueuedConnection
        void jobStarted(int jobId, QString fileName);
        void jobProgress(int jobId, float progress);
        void jobFinished(int jobId, bool success, QString message);
        void allJobsDone();

    private:
        void processJob(ConversionJob job);

        QList<std::shared_ptr<IConverter>> converters_;
        std::unique_ptr<ThreadPool>        threadPool_;
        std::atomic<int>                   activeJobs_{ 0 };
        std::atomic<bool>                  cancelled_{ false };
        std::atomic<int>                   nextJobId_{ 1 };
    };

} // namespace zc