#include "file_compressor.h"
#include "file_decompressor.h"
#include "main_window.h"

#include <QApplication>
#include <QStringList>
#include <iostream>

int main(int argc, char* argv[]) {
    // Support headless verification flags on the packaged .app binary while
    // launching the full Qt desktop GUI by default when double-clicked in Finder.
    if (argc == 4) {
        const std::string mode = argv[1];
        const std::string inputPath = argv[2];
        const std::string outputPath = argv[3];

        try {
            if (mode == "--compress") {
                FileCompressor compressor;
                CompressionStats stats = compressor.compressFile(inputPath, outputPath);
                std::cout << "[FileZipper] Compressed " << stats.originalFileSize
                          << " bytes -> " << stats.compressedFileSize << " bytes." << std::endl;
                return 0;
            }
            if (mode == "--decompress") {
                FileDecompressor decompressor;
                DecompressionStats stats = decompressor.decompressFile(inputPath, outputPath);
                std::cout << "[FileZipper] Decompressed " << stats.compressedFileSize
                          << " bytes -> " << stats.restoredFileSize << " bytes." << std::endl;
                return 0;
            }
        } catch (const std::exception& ex) {
            std::cerr << "[FileZipper Error] " << ex.what() << std::endl;
            return 1;
        }
    }

    QApplication app(argc, argv);
    app.setApplicationName("FileZipper");
    app.setOrganizationName("DSAProject");
    app.setApplicationVersion("1.0.0");

    MainWindow window;
    window.show();

    return app.exec();
}
