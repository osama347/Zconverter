#include "ConversionEngine.h"
#include <QDebug>
#include <QFileInfo>
#include <QElapsedTimer>

namespace zc {

    ConversionEngine::ConversionEngine(QObject* parent)
        : QObject(parent)
        , threadPool_(std::make_unique<ThreadPool>())
    {
        qDebug() << "[Engine] Started with"
            << threadPool_->threadCount()
            << "worker threads";
    }

    ConversionEngine::~ConversionEngine()
    {
        threadPool_->shutdown();
    }

    void ConversionEngine::registerConverter(
        std::shared_ptr<IConverter> converter)
    {
        converters_.append(std::move(converter));
        qDebug() << "[Engine] Registered converter:"
            << converters_.last()->name();
    }

    IConverter* ConversionEngine::findConverter(
        const QString& fromExt, const QString& toExt) const
    {
        for (auto& conv : converters_) {
            if (conv->canConvert(fromExt, toExt))
                return conv.get();
        }
        return nullptr;
    }

    void ConversionEngine::submitJob(ConversionJob job)
    {
        // Assign unique ID
        job.id = nextJobId_++;
        job.status = JobStatus::Pending;

        // Get file size
        QFileInfo info(job.inputPath);
        job.inputBytes = info.size();

        qDebug() << "[Engine] Submitting job" << job.id
            << ":" << job.inputFileName()
            << "->" << job.targetFormat;

        activeJobs_++;

        // Capture by value so job lives on the worker thread
        threadPool_->submit([this, job]() mutable {
            processJob(job);
            });
    }

    void ConversionEngine::processJob(ConversionJob job)
    {
        if (cancelled_) {
            job.status = JobStatus::Cancelled;
            activeJobs_--;
            return;
        }

        emit jobStarted(job.id, job.inputFileName());

        // Find the right converter
        IConverter* converter = findConverter(
            job.inputExtension(), job.targetFormat);

        if (!converter) {
            job.status = JobStatus::Failed;
            job.errorMessage = QString("No converter found for %1 → %2")
                .arg(job.inputExtension())
                .arg(job.targetFormat);

            qDebug() << "[Engine] Job" << job.id << "failed:"
                << job.errorMessage;

            emit jobFinished(job.id, false, job.errorMessage);
            activeJobs_--;

            if (activeJobs_ == 0)
                emit allJobsDone();
            return;
        }

        // Progress callback — emits signal back to UI
        auto progressCb = [this](int id, float p) {
            emit jobProgress(id, p);
            };

        QElapsedTimer timer;
        timer.start();

        bool ok = converter->convert(job, progressCb);

        job.durationMs = timer.elapsed();
        job.status = ok ? JobStatus::Done : JobStatus::Failed;

        qDebug() << "[Engine] Job" << job.id
            << (ok ? "done" : "failed")
            << "in" << job.durationMs << "ms";

        emit jobFinished(job.id, ok, job.errorMessage);
        activeJobs_--;

        if (activeJobs_ == 0)
            emit allJobsDone();
    }

    void ConversionEngine::cancelAll()
    {
        cancelled_ = true;
        qDebug() << "[Engine] All jobs cancelled";
    }

    bool ConversionEngine::isRunning() const
    {
        return activeJobs_ > 0;
    }

} // namespace zc