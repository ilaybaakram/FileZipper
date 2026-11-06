#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "file_compressor.h"
#include "file_decompressor.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFrame>
#include <QFutureWatcher>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QString>
#include <cstdint>

/**
 * @brief Result structure returned by background compression/decompression tasks.
 */
struct OperationResult {
    bool isCompressMode = true;
    bool succeeded = false;
    QString errorMessage;
    QString inputFilePath;
    QString outputFilePath;
    CompressionStats compressionStats{};
    DecompressionStats decompressionStats{};
};

/**
 * @brief Interactive drag-and-drop file selection card widget.
 */
class DropZoneFrame : public QFrame {
    Q_OBJECT

public:
    explicit DropZoneFrame(QWidget* parent = nullptr);

signals:
    void fileDropped(const QString& filePath);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
};

/**
 * @brief Modern dark-themed Qt desktop GUI for FileZipper.
 *
 * Integrates the existing huffman_core library (FileCompressor and FileDecompressor)
 * using asynchronous background execution (QtConcurrent + QFutureWatcher) so the
 * UI remains responsive during large file operations.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    // Public helper methods for programmatic verification & testing
    void selectInputFile(const QString& filePath);
    void setOperationMode(bool compressMode);
    void setOutputDirectory(const QString& dirPath);
    void setOutputFileName(const QString& fileName);
    void triggerStartOperation();
    bool isProcessing() const;
    OperationResult getLastResult() const;
    static QString getInputFileDialogFilter();

private slots:
    void onBrowseInputClicked();
    void onClearInputClicked();
    void onFileDropped(const QString& filePath);
    void onModeToggled();
    void onBrowseOutputDirClicked();
    void onOutputFieldsEdited();
    void onRunActionClicked();
    void onOpenOutputFolderClicked();
    void onResetForAnotherClicked();
    void onBackgroundTaskFinished();

private:
    void setupUi();
    void applyDarkTheme();
    QPixmap createAppLogoPixmap(int size) const;
    void updateSuggestedOutput();
    void updateActionState();
    void setControlsEnabledForProcessing(bool processing);
    void displayResultStats(const OperationResult& result);
    static QString formatByteSize(std::uint64_t bytes);

    // State
    QString selectedFilePath_;
    std::uint64_t selectedFileSize_;
    bool isProcessing_;
    OperationResult lastResult_;

    // Background concurrency watcher
    QFutureWatcher<OperationResult>* taskWatcher_;

    // Header widgets
    QLabel* logoLabel_;
    QLabel* titleLabel_;
    QLabel* subtitleLabel_;

    // Input drop zone widgets
    DropZoneFrame* dropZone_;
    QLabel* dropIconLabel_;
    QLabel* fileNameLabel_;
    QLabel* filePathLabel_;
    QLabel* fileSizeBadge_;
    QPushButton* browseFileBtn_;
    QPushButton* clearFileBtn_;

    // Operation mode widgets
    QRadioButton* compressRadio_;
    QRadioButton* decompressRadio_;

    // Output configuration widgets
    QLineEdit* outputDirEdit_;
    QPushButton* browseOutputDirBtn_;
    QLineEdit* outputFileNameEdit_;
    QLabel* fullOutputPreviewLabel_;

    // Action & progress widgets
    QPushButton* actionBtn_;
    QProgressBar* progressBar_;

    // Statistics panel widgets
    QFrame* statsCard_;
    QLabel* statTitle1_;
    QLabel* statValue1_;
    QLabel* statTitle2_;
    QLabel* statValue2_;
    QLabel* statTitle3_;
    QLabel* statValue3_;
    QLabel* statTitle4_;
    QLabel* statValue4_;

    // Completion / status banner widgets
    QFrame* statusBanner_;
    QLabel* statusBannerTitle_;
    QLabel* statusBannerDetail_;
    QPushButton* openFolderBtn_;
    QPushButton* resetAnotherBtn_;
};

#endif // MAIN_WINDOW_H

