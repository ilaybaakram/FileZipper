#include "main_window.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void waitForGuiTask(MainWindow& window, int timeoutMs = 10000) {
    QElapsedTimer timer;
    timer.start();
    while (window.isProcessing() && timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    QCoreApplication::processEvents();
    assert(!window.isProcessing());
}

void writeFileBytes(const std::string& path, const std::string& content) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    assert(out.is_open());
    out.write(content.data(), static_cast<std::streamsize>(content.size()));
    out.close();
}

std::string readFileBytes(const std::string& path) {
    std::ifstream in(path, std::ios::binary | std::ios::ate);
    assert(in.is_open());
    const std::streamsize size = in.tellg();
    in.seekg(0, std::ios::beg);
    std::string buffer(static_cast<std::size_t>(size), '\0');
    if (size > 0) {
        assert(in.read(&buffer[0], size));
    }
    return buffer;
}

} // namespace

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::cout << "=== FileZipper: Qt 6 Desktop GUI Integration Tests ===" << std::endl;

    MainWindow window;
    window.show();
    QCoreApplication::processEvents();
    assert(window.isVisible());
    std::cout << "[PASS] Test 1: MainWindow launched and rendered successfully." << std::endl;

    const QString workDir = QDir::currentPath();

    // -------------------------------------------------------------------------
    // Test 2: GUI Compression & Decompression round-trip (byte-for-byte match)
    // -------------------------------------------------------------------------
    {
        const std::string inFile = (workDir + "/temp_gui_sample.txt").toStdString();
        const std::string hufFile = (workDir + "/temp_gui_sample.txt.huf").toStdString();
        const std::string restoredFile = (workDir + "/restored_temp_gui_sample.txt").toStdString();

        std::string sampleContent;
        for (int i = 0; i < 200; ++i) {
            sampleContent += "Qt 6 Desktop GUI + C++17 Huffman Coding Compression Engine Test Line!\n";
        }
        writeFileBytes(inFile, sampleContent);

        // Step A: Select input file and trigger Compress through GUI
        window.selectInputFile(QString::fromStdString(inFile));
        window.setOperationMode(true);
        window.setOutputDirectory(workDir);
        window.setOutputFileName("temp_gui_sample.txt.huf");
        window.triggerStartOperation();
        waitForGuiTask(window);

        OperationResult compRes = window.getLastResult();
        assert(compRes.succeeded == true);
        assert(compRes.isCompressMode == true);
        assert(compRes.compressionStats.originalFileSize == sampleContent.size());
        assert(compRes.compressionStats.compressedFileSize < compRes.compressionStats.originalFileSize);
        assert(QFileInfo::exists(QString::fromStdString(hufFile)));

        // Step B: Select the generated .huf archive and trigger Decompress through GUI
        window.selectInputFile(QString::fromStdString(hufFile));
        window.setOperationMode(false);
        window.setOutputDirectory(workDir);
        window.setOutputFileName("restored_temp_gui_sample.txt");
        window.triggerStartOperation();
        waitForGuiTask(window);

        OperationResult decompRes = window.getLastResult();
        assert(decompRes.succeeded == true);
        assert(decompRes.isCompressMode == false);
        assert(decompRes.decompressionStats.restoredFileSize == sampleContent.size());

        const std::string restoredBytes = readFileBytes(restoredFile);
        assert(restoredBytes == sampleContent);

        std::remove(inFile.c_str());
        std::remove(hufFile.c_str());
        std::remove(restoredFile.c_str());
        std::cout << "[PASS] Test 2: GUI Compression & Decompression round-trip verified byte-for-byte." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 3: Empty file compression and decompression through GUI
    // -------------------------------------------------------------------------
    {
        const std::string emptyIn = (workDir + "/temp_gui_empty.txt").toStdString();
        const std::string emptyHuf = (workDir + "/temp_gui_empty.txt.huf").toStdString();
        const std::string emptyRestored = (workDir + "/restored_temp_gui_empty.txt").toStdString();

        writeFileBytes(emptyIn, "");

        window.selectInputFile(QString::fromStdString(emptyIn));
        window.setOperationMode(true);
        window.setOutputDirectory(workDir);
        window.setOutputFileName("temp_gui_empty.txt.huf");
        window.triggerStartOperation();
        waitForGuiTask(window);

        OperationResult compRes = window.getLastResult();
        assert(compRes.succeeded == true);
        assert(compRes.compressionStats.originalFileSize == 0);

        window.selectInputFile(QString::fromStdString(emptyHuf));
        window.setOperationMode(false);
        window.setOutputDirectory(workDir);
        window.setOutputFileName("restored_temp_gui_empty.txt");
        window.triggerStartOperation();
        waitForGuiTask(window);

        OperationResult decompRes = window.getLastResult();
        assert(decompRes.succeeded == true);
        assert(decompRes.decompressionStats.restoredFileSize == 0);
        assert(readFileBytes(emptyRestored).empty());

        std::remove(emptyIn.c_str());
        std::remove(emptyHuf.c_str());
        std::remove(emptyRestored.c_str());
        std::cout << "[PASS] Test 3: GUI Empty file compression and decompression verified." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 4: Invalid input path & corrupted .huf archive handling in GUI
    // -------------------------------------------------------------------------
    {
        // Non-existent input path handled gracefully without crashing
        window.selectInputFile(workDir + "/does_not_exist_99999.txt");
        QCoreApplication::processEvents();

        // Invalid .huf archive handled gracefully with user-friendly error message
        const std::string invalidHuf = (workDir + "/temp_gui_invalid.huf").toStdString();
        const std::string invalidOut = (workDir + "/temp_gui_invalid_out.txt").toStdString();
        writeFileBytes(invalidHuf, "NOT_A_VALID_HUF_FILE");

        window.selectInputFile(QString::fromStdString(invalidHuf));
        window.setOperationMode(false);
        window.setOutputDirectory(workDir);
        window.setOutputFileName("temp_gui_invalid_out.txt");
        window.triggerStartOperation();
        waitForGuiTask(window);

        OperationResult badRes = window.getLastResult();
        assert(badRes.succeeded == false);
        assert(!badRes.errorMessage.isEmpty());

        std::remove(invalidHuf.c_str());
        std::remove(invalidOut.c_str());
        std::cout << "[PASS] Test 4: Invalid input path and corrupted archive handled gracefully in GUI." << std::endl;
    }

    // -------------------------------------------------------------------------
    // Test 5: GUI Image File Selection Filter & Lossless Image Round-Trips
    //         (.png, .jpg, .bmp, .gif, .webp, .tiff, .ico)
    // -------------------------------------------------------------------------
    {
        // Verify file dialog filter supports all files, common image formats, and .huf archives
        const QString filter = MainWindow::getInputFileDialogFilter();
        assert(filter.contains("All Files (*)"));
        assert(filter.contains("*.jpg"));
        assert(filter.contains("*.jpeg"));
        assert(filter.contains("*.png"));
        assert(filter.contains("*.bmp"));
        assert(filter.contains("*.gif"));
        assert(filter.contains("*.webp"));
        assert(filter.contains("*.tiff"));
        assert(filter.contains("*.ico"));
        assert(filter.contains("*.huf"));

        // Create a deterministic 48x48 RGBA test pattern QImage
        QImage testImg(48, 48, QImage::Format_ARGB32);
        for (int y = 0; y < testImg.height(); ++y) {
            for (int x = 0; x < testImg.width(); ++x) {
                testImg.setPixelColor(x, y, QColor((x * 5) % 256, (y * 5) % 256, 180, 255));
            }
        }

        const QStringList imageFormats = {"png", "jpg", "bmp"};
        for (const QString& ext : imageFormats) {
            const QString origName = QString("temp_gui_image.%1").arg(ext);
            const QString hufName = origName + ".huf";
            const QString restoredName = "restored_" + origName;

            const QString origPath = workDir + "/" + origName;
            const QString hufPath = workDir + "/" + hufName;
            const QString restoredPath = workDir + "/" + restoredName;

            assert(testImg.save(origPath, ext.toUpper().toUtf8().constData()));
            const std::string originalBytes = readFileBytes(origPath.toStdString());
            assert(!originalBytes.empty());

            // Start from Decompress mode first to verify selecting an image switches to Compress mode
            window.setOperationMode(false);
            window.selectInputFile(origPath);
            window.setOutputDirectory(workDir);
            window.setOutputFileName(hufName);
            window.triggerStartOperation();
            waitForGuiTask(window);

            OperationResult compRes = window.getLastResult();
            assert(compRes.succeeded == true);
            assert(compRes.isCompressMode == true);
            assert(compRes.compressionStats.originalFileSize == originalBytes.size());
            assert(QFileInfo::exists(hufPath));

            // Select the .huf archive and decompress via GUI
            window.selectInputFile(hufPath);
            window.setOutputDirectory(workDir);
            window.setOutputFileName(restoredName);
            window.triggerStartOperation();
            waitForGuiTask(window);

            OperationResult decompRes = window.getLastResult();
            assert(decompRes.succeeded == true);
            assert(decompRes.isCompressMode == false);
            assert(decompRes.decompressionStats.restoredFileSize == originalBytes.size());

            // Verify byte-for-byte identity via memory comparison and system 'cmp -s'
            const std::string restoredBytes = readFileBytes(restoredPath.toStdString());
            assert(restoredBytes == originalBytes);
            const std::string cmpCmd =
                "cmp -s \"" + origPath.toStdString() + "\" \"" + restoredPath.toStdString() + "\"";
            assert(std::system(cmpCmd.c_str()) == 0);

            // Verify the restored image loads cleanly in QImage with identical dimensions
            QImage restoredImg(restoredPath);
            assert(!restoredImg.isNull());
            assert(restoredImg.size() == testImg.size());

            QFile::remove(origPath);
            QFile::remove(hufPath);
            QFile::remove(restoredPath);
        }

        std::cout << "[PASS] Test 5: GUI image filter and PNG/JPG/BMP round-trips verified byte-for-byte."
                  << std::endl;
    }

    window.close();
    std::cout << "All Qt 6 GUI tests passed successfully!" << std::endl;
    return 0;
}

