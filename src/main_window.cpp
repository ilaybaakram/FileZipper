#include "main_window.h"

#include <QApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QMimeData>
#include <QPainter>
#include <QPainterPath>
#include <QStyle>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <exception>

// ============================================================================
// DropZoneFrame Implementation
// ============================================================================

DropZoneFrame::DropZoneFrame(QWidget* parent) : QFrame(parent) {
    setAcceptDrops(true);
    setObjectName("dropZone");
    setProperty("dragActive", false);
}

void DropZoneFrame::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        const QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.isEmpty() && urls.first().isLocalFile()) {
            QFileInfo info(urls.first().toLocalFile());
            if (info.isFile()) {
                setProperty("dragActive", true);
                style()->unpolish(this);
                style()->polish(this);
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void DropZoneFrame::dragLeaveEvent(QDragLeaveEvent* event) {
    setProperty("dragActive", false);
    style()->unpolish(this);
    style()->polish(this);
    QFrame::dragLeaveEvent(event);
}

void DropZoneFrame::dropEvent(QDropEvent* event) {
    setProperty("dragActive", false);
    style()->unpolish(this);
    style()->polish(this);

    const QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty() && urls.first().isLocalFile()) {
        const QString localPath = urls.first().toLocalFile();
        QFileInfo info(localPath);
        if (info.isFile()) {
            emit fileDropped(localPath);
            event->acceptProposedAction();
        }
    }
}

// ============================================================================
// MainWindow Implementation
// ============================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      selectedFileSize_(0),
      isProcessing_(false),
      taskWatcher_(new QFutureWatcher<OperationResult>(this)) {
    setupUi();
    applyDarkTheme();

    connect(taskWatcher_, &QFutureWatcher<OperationResult>::finished,
            this, &MainWindow::onBackgroundTaskFinished);

    updateActionState();
}

MainWindow::~MainWindow() {
    if (taskWatcher_->isRunning()) {
        taskWatcher_->waitForFinished();
    }
}

QPixmap MainWindow::createAppLogoPixmap(int size) const {
    const QStringList candidatePaths = {
        QCoreApplication::applicationDirPath() + "/../Resources/app_icon.png",
        QCoreApplication::applicationDirPath() + "/assets/app_icon.png",
        QCoreApplication::applicationDirPath() + "/../assets/app_icon.png",
        "assets/app_icon.png"
    };
    for (const QString& path : candidatePaths) {
        if (QFileInfo::exists(path)) {
            QPixmap customIcon(path);
            if (!customIcon.isNull()) {
                return customIcon.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            }
        }
    }

    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Rounded gradient background badge
    QLinearGradient bgGrad(0, 0, size, size);
    bgGrad.setColorAt(0.0, QColor("#3B82F6"));
    bgGrad.setColorAt(1.0, QColor("#1D4ED8"));

    QPainterPath rectPath;
    rectPath.addRoundedRect(QRectF(2, 2, size - 4, size - 4), 12.0, 12.0);
    painter.fillPath(rectPath, bgGrad);

    // Draw stylized zipper teeth + Huffman binary tree node motif in crisp white
    QPen linePen(QColor("#EFF6FF"), 2.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(linePen);

    const double cx = size * 0.5;
    const double topY = size * 0.26;
    const double midY = size * 0.52;
    const double botY = size * 0.74;
    const double leftX = size * 0.28;
    const double rightX = size * 0.72;

    // Binary tree edges: root -> left, root -> right, right -> grandchildren
    painter.drawLine(QPointF(cx, topY), QPointF(leftX, midY));
    painter.drawLine(QPointF(cx, topY), QPointF(rightX, midY));
    painter.drawLine(QPointF(rightX, midY), QPointF(cx, botY));
    painter.drawLine(QPointF(rightX, midY), QPointF(rightX + size * 0.08, botY));

    // Draw tree nodes
    painter.setBrush(QColor("#FFFFFF"));
    painter.setPen(QPen(QColor("#1E3A8A"), 1.5));
    const double r = size * 0.075;
    painter.drawEllipse(QPointF(cx, topY), r, r);
    painter.drawEllipse(QPointF(leftX, midY), r, r);
    painter.drawEllipse(QPointF(rightX, midY), r, r);
    painter.drawEllipse(QPointF(cx, botY), r * 0.85, r * 0.85);
    painter.drawEllipse(QPointF(rightX + size * 0.08, botY), r * 0.85, r * 0.85);

    return pixmap;
}

void MainWindow::setupUi() {
    setWindowTitle("File Zipper — Huffman Compression Utility");
    setMinimumSize(760, 740);
    resize(820, 780);
    setWindowIcon(QIcon(createAppLogoPixmap(64)));

    QWidget* central = new QWidget(this);
    central->setObjectName("centralContainer");
    setCentralWidget(central);

    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(28, 22, 28, 22);
    mainLayout->setSpacing(14);

    // ------------------------------------------------------------------------
    // 1. Header Section
    // ------------------------------------------------------------------------
    QHBoxLayout* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(14);

    logoLabel_ = new QLabel(central);
    logoLabel_->setPixmap(createAppLogoPixmap(46));
    logoLabel_->setFixedSize(46, 46);

    QVBoxLayout* titleColumn = new QVBoxLayout();
    titleColumn->setSpacing(2);
    titleLabel_ = new QLabel("File Zipper", central);
    titleLabel_->setObjectName("appTitle");
    subtitleLabel_ = new QLabel("Huffman Coding — Lossless File Compression", central);
    subtitleLabel_->setObjectName("appSubtitle");
    titleColumn->addWidget(titleLabel_);
    titleColumn->addWidget(subtitleLabel_);

    headerLayout->addWidget(logoLabel_);
    headerLayout->addLayout(titleColumn);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // ------------------------------------------------------------------------
    // 2. File Selection Card (Drag & Drop + Browse + Clear)
    // ------------------------------------------------------------------------
    dropZone_ = new DropZoneFrame(central);
    QVBoxLayout* dropLayout = new QVBoxLayout(dropZone_);
    dropLayout->setContentsMargins(20, 16, 20, 16);
    dropLayout->setSpacing(8);

    QHBoxLayout* topRow = new QHBoxLayout();
    dropIconLabel_ = new QLabel("FILE SELECTION", dropZone_);
    dropIconLabel_->setObjectName("sectionTag");

    fileSizeBadge_ = new QLabel("No file selected", dropZone_);
    fileSizeBadge_->setObjectName("sizeBadge");

    topRow->addWidget(dropIconLabel_);
    topRow->addStretch();
    topRow->addWidget(fileSizeBadge_);
    dropLayout->addLayout(topRow);

    fileNameLabel_ = new QLabel("Drag & drop a file here, or click Browse File", dropZone_);
    fileNameLabel_->setObjectName("fileNamePrimary");
    fileNameLabel_->setWordWrap(true);

    filePathLabel_ = new QLabel("Supports any text or binary file for compression, and .huf archives for decompression.", dropZone_);
    filePathLabel_->setObjectName("filePathSecondary");
    filePathLabel_->setWordWrap(true);

    dropLayout->addWidget(fileNameLabel_);
    dropLayout->addWidget(filePathLabel_);

    QHBoxLayout* fileButtonsLayout = new QHBoxLayout();
    fileButtonsLayout->setSpacing(10);

    browseFileBtn_ = new QPushButton("Browse File...", dropZone_);
    browseFileBtn_->setObjectName("secondaryButton");
    browseFileBtn_->setCursor(Qt::PointingHandCursor);

    clearFileBtn_ = new QPushButton("Clear Selection", dropZone_);
    clearFileBtn_->setObjectName("ghostButton");
    clearFileBtn_->setCursor(Qt::PointingHandCursor);
    clearFileBtn_->setEnabled(false);

    fileButtonsLayout->addWidget(browseFileBtn_);
    fileButtonsLayout->addWidget(clearFileBtn_);
    fileButtonsLayout->addStretch();
    dropLayout->addLayout(fileButtonsLayout);

    mainLayout->addWidget(dropZone_);

    // ------------------------------------------------------------------------
    // 3. Operation Mode & Output Configuration Card
    // ------------------------------------------------------------------------
    QFrame* configCard = new QFrame(central);
    configCard->setObjectName("cardPanel");
    QVBoxLayout* configLayout = new QVBoxLayout(configCard);
    configLayout->setContentsMargins(20, 16, 20, 16);
    configLayout->setSpacing(12);

    QLabel* modeHeader = new QLabel("OPERATION & OUTPUT SETTINGS", configCard);
    modeHeader->setObjectName("sectionTag");
    configLayout->addWidget(modeHeader);

    QHBoxLayout* modeRow = new QHBoxLayout();
    modeRow->setSpacing(16);

    compressRadio_ = new QRadioButton("Compress File (Create .huf Archive)", configCard);
    compressRadio_->setObjectName("modeRadio");
    compressRadio_->setChecked(true);
    compressRadio_->setCursor(Qt::PointingHandCursor);

    decompressRadio_ = new QRadioButton("Decompress Archive (Restore from .huf)", configCard);
    decompressRadio_->setObjectName("modeRadio");
    decompressRadio_->setCursor(Qt::PointingHandCursor);

    modeRow->addWidget(compressRadio_);
    modeRow->addWidget(decompressRadio_);
    modeRow->addStretch();
    configLayout->addLayout(modeRow);

    QGridLayout* outputGrid = new QGridLayout();
    outputGrid->setHorizontalSpacing(10);
    outputGrid->setVerticalSpacing(8);

    QLabel* outDirLabel = new QLabel("Output Folder:", configCard);
    outDirLabel->setObjectName("fieldLabel");
    outputDirEdit_ = new QLineEdit(configCard);
    outputDirEdit_->setPlaceholderText("Select destination directory...");
    outputDirEdit_->setObjectName("textField");

    browseOutputDirBtn_ = new QPushButton("Choose Folder...", configCard);
    browseOutputDirBtn_->setObjectName("secondaryButton");
    browseOutputDirBtn_->setCursor(Qt::PointingHandCursor);

    QLabel* outNameLabel = new QLabel("Output Filename:", configCard);
    outNameLabel->setObjectName("fieldLabel");
    outputFileNameEdit_ = new QLineEdit(configCard);
    outputFileNameEdit_->setPlaceholderText("Suggested output filename...");
    outputFileNameEdit_->setObjectName("textField");

    outputGrid->addWidget(outDirLabel, 0, 0);
    outputGrid->addWidget(outputDirEdit_, 0, 1);
    outputGrid->addWidget(browseOutputDirBtn_, 0, 2);
    outputGrid->addWidget(outNameLabel, 1, 0);
    outputGrid->addWidget(outputFileNameEdit_, 1, 1, 1, 2);

    configLayout->addLayout(outputGrid);

    fullOutputPreviewLabel_ = new QLabel("Destination: —", configCard);
    fullOutputPreviewLabel_->setObjectName("filePathSecondary");
    fullOutputPreviewLabel_->setWordWrap(true);
    configLayout->addWidget(fullOutputPreviewLabel_);

    mainLayout->addWidget(configCard);

    // ------------------------------------------------------------------------
    // 4. Primary Action Button & Indeterminate Progress Bar
    // ------------------------------------------------------------------------
    actionBtn_ = new QPushButton("Compress File", central);
    actionBtn_->setObjectName("primaryActionButton");
    actionBtn_->setFixedHeight(44);
    actionBtn_->setCursor(Qt::PointingHandCursor);
    mainLayout->addWidget(actionBtn_);

    progressBar_ = new QProgressBar(central);
    progressBar_->setObjectName("operationProgress");
    progressBar_->setRange(0, 0); // Indeterminate animation while running
    progressBar_->setTextVisible(false);
    progressBar_->setFixedHeight(6);
    progressBar_->setVisible(false);
    mainLayout->addWidget(progressBar_);

    // ------------------------------------------------------------------------
    // 5. Statistics Panel Card
    // ------------------------------------------------------------------------
    statsCard_ = new QFrame(central);
    statsCard_->setObjectName("cardPanel");
    QVBoxLayout* statsOuterLayout = new QVBoxLayout(statsCard_);
    statsOuterLayout->setContentsMargins(20, 14, 20, 14);
    statsOuterLayout->setSpacing(10);

    QLabel* statsHeader = new QLabel("OPERATION STATISTICS", statsCard_);
    statsHeader->setObjectName("sectionTag");
    statsOuterLayout->addWidget(statsHeader);

    QGridLayout* statsGrid = new QGridLayout();
    statsGrid->setHorizontalSpacing(18);
    statsGrid->setVerticalSpacing(4);

    statTitle1_ = new QLabel("Original Size", statsCard_);
    statTitle1_->setObjectName("statTitle");
    statValue1_ = new QLabel("—", statsCard_);
    statValue1_->setObjectName("statValue");

    statTitle2_ = new QLabel("Compressed Size", statsCard_);
    statTitle2_->setObjectName("statTitle");
    statValue2_ = new QLabel("—", statsCard_);
    statValue2_->setObjectName("statValue");

    statTitle3_ = new QLabel("Compression Ratio", statsCard_);
    statTitle3_->setObjectName("statTitle");
    statValue3_ = new QLabel("—", statsCard_);
    statValue3_->setObjectName("statValue");

    statTitle4_ = new QLabel("Space Saved", statsCard_);
    statTitle4_->setObjectName("statTitle");
    statValue4_ = new QLabel("—", statsCard_);
    statValue4_->setObjectName("statValue");

    statsGrid->addWidget(statTitle1_, 0, 0);
    statsGrid->addWidget(statTitle2_, 0, 1);
    statsGrid->addWidget(statTitle3_, 0, 2);
    statsGrid->addWidget(statTitle4_, 0, 3);

    statsGrid->addWidget(statValue1_, 1, 0);
    statsGrid->addWidget(statValue2_, 1, 1);
    statsGrid->addWidget(statValue3_, 1, 2);
    statsGrid->addWidget(statValue4_, 1, 3);

    statsOuterLayout->addLayout(statsGrid);
    mainLayout->addWidget(statsCard_);

    // ------------------------------------------------------------------------
    // 6. Completion / Status Banner Card
    // ------------------------------------------------------------------------
    statusBanner_ = new QFrame(central);
    statusBanner_->setObjectName("statusBanner");
    statusBanner_->setProperty("bannerState", "idle");

    QVBoxLayout* bannerLayout = new QVBoxLayout(statusBanner_);
    bannerLayout->setContentsMargins(18, 12, 18, 12);
    bannerLayout->setSpacing(6);

    statusBannerTitle_ = new QLabel("Ready", statusBanner_);
    statusBannerTitle_->setObjectName("bannerTitle");

    statusBannerDetail_ = new QLabel("Select a file above to begin lossless Huffman compression or decompression.", statusBanner_);
    statusBannerDetail_->setObjectName("bannerDetail");
    statusBannerDetail_->setWordWrap(true);

    QHBoxLayout* bannerBtnRow = new QHBoxLayout();
    bannerBtnRow->setSpacing(10);

    openFolderBtn_ = new QPushButton("Open Output Folder", statusBanner_);
    openFolderBtn_->setObjectName("secondaryButton");
    openFolderBtn_->setCursor(Qt::PointingHandCursor);
    openFolderBtn_->setVisible(false);

    resetAnotherBtn_ = new QPushButton("Perform Another Operation", statusBanner_);
    resetAnotherBtn_->setObjectName("ghostButton");
    resetAnotherBtn_->setCursor(Qt::PointingHandCursor);
    resetAnotherBtn_->setVisible(false);

    bannerBtnRow->addWidget(openFolderBtn_);
    bannerBtnRow->addWidget(resetAnotherBtn_);
    bannerBtnRow->addStretch();

    bannerLayout->addWidget(statusBannerTitle_);
    bannerLayout->addWidget(statusBannerDetail_);
    bannerLayout->addLayout(bannerBtnRow);

    mainLayout->addWidget(statusBanner_);
    mainLayout->addStretch();

    // Wire up signals & slots
    connect(browseFileBtn_, &QPushButton::clicked, this, &MainWindow::onBrowseInputClicked);
    connect(clearFileBtn_, &QPushButton::clicked, this, &MainWindow::onClearInputClicked);
    connect(dropZone_, &DropZoneFrame::fileDropped, this, &MainWindow::onFileDropped);
    connect(compressRadio_, &QRadioButton::toggled, this, &MainWindow::onModeToggled);
    connect(browseOutputDirBtn_, &QPushButton::clicked, this, &MainWindow::onBrowseOutputDirClicked);
    connect(outputDirEdit_, &QLineEdit::textChanged, this, &MainWindow::onOutputFieldsEdited);
    connect(outputFileNameEdit_, &QLineEdit::textChanged, this, &MainWindow::onOutputFieldsEdited);
    connect(actionBtn_, &QPushButton::clicked, this, &MainWindow::onRunActionClicked);
    connect(openFolderBtn_, &QPushButton::clicked, this, &MainWindow::onOpenOutputFolderClicked);
    connect(resetAnotherBtn_, &QPushButton::clicked, this, &MainWindow::onResetForAnotherClicked);
}

void MainWindow::applyDarkTheme() {
    const QString qss = R"(
        QMainWindow, QWidget#centralContainer {
            background-color: #0F172A;
            color: #F8FAFC;
            font-family: -apple-system, "Segoe UI", "Helvetica Neue", Arial, sans-serif;
        }
        QLabel#appTitle {
            font-size: 22px;
            font-weight: 700;
            color: #F8FAFC;
        }
        QLabel#appSubtitle {
            font-size: 13px;
            color: #94A3B8;
        }
        QLabel#sectionTag {
            font-size: 11px;
            font-weight: 700;
            letter-spacing: 1px;
            color: #60A5FA;
        }
        QLabel#sizeBadge {
            background-color: #1E293B;
            color: #CBD5E1;
            border: 1px solid #334155;
            border-radius: 6px;
            padding: 3px 10px;
            font-size: 12px;
            font-weight: 600;
        }
        QFrame#dropZone {
            background-color: #1E293B;
            border: 2px dashed #334155;
            border-radius: 12px;
        }
        QFrame#dropZone[dragActive="true"] {
            background-color: #1E3A8A;
            border: 2px dashed #60A5FA;
        }
        QFrame#cardPanel {
            background-color: #1E293B;
            border: 1px solid #334155;
            border-radius: 12px;
        }
        QLabel#fileNamePrimary {
            font-size: 15px;
            font-weight: 600;
            color: #F1F5F9;
        }
        QLabel#filePathSecondary {
            font-size: 12px;
            color: #94A3B8;
        }
        QLabel#fieldLabel {
            font-size: 13px;
            font-weight: 500;
            color: #CBD5E1;
        }
        QRadioButton#modeRadio {
            font-size: 13px;
            font-weight: 600;
            color: #E2E8F0;
            spacing: 8px;
        }
        QLineEdit#textField {
            background-color: #0F172A;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 7px 11px;
            color: #F8FAFC;
            font-size: 13px;
        }
        QLineEdit#textField:focus {
            border: 1px solid #3B82F6;
        }
        QPushButton#primaryActionButton {
            background-color: #2563EB;
            color: #FFFFFF;
            border: none;
            border-radius: 10px;
            font-size: 15px;
            font-weight: 700;
            padding: 8px 18px;
        }
        QPushButton#primaryActionButton:hover {
            background-color: #3B82F6;
        }
        QPushButton#primaryActionButton:disabled {
            background-color: #1E293B;
            color: #475569;
            border: 1px solid #334155;
        }
        QPushButton#secondaryButton {
            background-color: #334155;
            color: #F8FAFC;
            border: 1px solid #475569;
            border-radius: 8px;
            padding: 6px 14px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton#secondaryButton:hover {
            background-color: #475569;
        }
        QPushButton#secondaryButton:disabled {
            background-color: #1E293B;
            color: #475569;
            border: 1px solid #334155;
        }
        QPushButton#ghostButton {
            background-color: transparent;
            color: #94A3B8;
            border: 1px solid #334155;
            border-radius: 8px;
            padding: 6px 14px;
            font-size: 12px;
            font-weight: 600;
        }
        QPushButton#ghostButton:hover {
            color: #F8FAFC;
            border-color: #64748B;
        }
        QPushButton#ghostButton:disabled {
            color: #475569;
            border-color: #1E293B;
        }
        QLabel#statTitle {
            font-size: 12px;
            color: #94A3B8;
        }
        QLabel#statValue {
            font-size: 16px;
            font-weight: 700;
            color: #F8FAFC;
        }
        QFrame#statusBanner {
            background-color: #1E293B;
            border: 1px solid #334155;
            border-radius: 12px;
        }
        QFrame#statusBanner[bannerState="success"] {
            background-color: #064E3B;
            border: 1px solid #10B981;
        }
        QFrame#statusBanner[bannerState="error"] {
            background-color: #450A0A;
            border: 1px solid #EF4444;
        }
        QLabel#bannerTitle {
            font-size: 14px;
            font-weight: 700;
            color: #F8FAFC;
        }
        QLabel#bannerDetail {
            font-size: 12px;
            color: #E2E8F0;
        }
        QProgressBar#operationProgress {
            background-color: #1E293B;
            border: none;
            border-radius: 3px;
        }
        QProgressBar#operationProgress::chunk {
            background-color: #3B82F6;
            border-radius: 3px;
        }
    )";
    setStyleSheet(qss);
}

QString MainWindow::formatByteSize(std::uint64_t bytes) {
    if (bytes < 1024ULL) {
        return QString("%1 B").arg(bytes);
    }
    if (bytes < 1024ULL * 1024ULL) {
        double kb = static_cast<double>(bytes) / 1024.0;
        return QString("%1 KB (%2 B)").arg(kb, 0, 'f', 2).arg(bytes);
    }
    double mb = static_cast<double>(bytes) / (1024.0 * 1024.0);
    return QString("%1 MB (%2 B)").arg(mb, 0, 'f', 2).arg(bytes);
}

void MainWindow::selectInputFile(const QString& filePath) {
    QFileInfo info(filePath);
    if (!info.exists() || !info.isFile()) {
        statusBanner_->setProperty("bannerState", "error");
        style()->unpolish(statusBanner_);
        style()->polish(statusBanner_);
        statusBannerTitle_->setText("Invalid Input File");
        statusBannerDetail_->setText("The selected file does not exist or is not a regular file: " + filePath);
        openFolderBtn_->setVisible(false);
        resetAnotherBtn_->setVisible(false);
        return;
    }

    selectedFilePath_ = info.absoluteFilePath();
    selectedFileSize_ = static_cast<std::uint64_t>(info.size());

    fileNameLabel_->setText(info.fileName());
    filePathLabel_->setText(selectedFilePath_);
    fileSizeBadge_->setText(formatByteSize(selectedFileSize_));
    clearFileBtn_->setEnabled(true);

    // Automatically select Decompress if the user picks a .huf file, otherwise Compress
    if (info.suffix().compare("huf", Qt::CaseInsensitive) == 0) {
        decompressRadio_->setChecked(true);
    } else {
        compressRadio_->setChecked(true);
    }

    outputDirEdit_->setText(info.absolutePath());
    updateSuggestedOutput();
    updateActionState();
}

void MainWindow::setOperationMode(bool compressMode) {
    if (compressMode) {
        compressRadio_->setChecked(true);
    } else {
        decompressRadio_->setChecked(true);
    }
    updateSuggestedOutput();
    updateActionState();
}

void MainWindow::setOutputDirectory(const QString& dirPath) {
    outputDirEdit_->setText(dirPath);
    onOutputFieldsEdited();
}

void MainWindow::setOutputFileName(const QString& fileName) {
    outputFileNameEdit_->setText(fileName);
    onOutputFieldsEdited();
}

void MainWindow::triggerStartOperation() {
    onRunActionClicked();
}

bool MainWindow::isProcessing() const {
    return isProcessing_;
}

OperationResult MainWindow::getLastResult() const {
    return lastResult_;
}

void MainWindow::onBrowseInputClicked() {
    const QString filter = compressRadio_->isChecked()
                               ? "All Files (*.*)"
                               : "Huffman Archives (*.huf);;All Files (*.*)";
    const QString chosen = QFileDialog::getOpenFileName(
        this,
        "Select Input File",
        outputDirEdit_->text().isEmpty() ? QDir::homePath() : outputDirEdit_->text(),
        filter
    );
    if (!chosen.isEmpty()) {
        selectInputFile(chosen);
    }
}

void MainWindow::onClearInputClicked() {
    selectedFilePath_.clear();
    selectedFileSize_ = 0;

    fileNameLabel_->setText("Drag & drop a file here, or click Browse File");
    filePathLabel_->setText("Supports any text or binary file for compression, and .huf archives for decompression.");
    fileSizeBadge_->setText("No file selected");
    clearFileBtn_->setEnabled(false);

    outputFileNameEdit_->clear();
    fullOutputPreviewLabel_->setText("Destination: —");

    statValue1_->setText("—");
    statValue2_->setText("—");
    statValue3_->setText("—");
    statValue4_->setText("—");

    statusBanner_->setProperty("bannerState", "idle");
    style()->unpolish(statusBanner_);
    style()->polish(statusBanner_);
    statusBannerTitle_->setText("Ready");
    statusBannerDetail_->setText("Select a file above to begin lossless Huffman compression or decompression.");
    openFolderBtn_->setVisible(false);
    resetAnotherBtn_->setVisible(false);

    updateActionState();
}

void MainWindow::onFileDropped(const QString& filePath) {
    selectInputFile(filePath);
}

void MainWindow::onModeToggled() {
    const bool isCompress = compressRadio_->isChecked();
    if (isCompress) {
        statTitle1_->setText("Original Size");
        statTitle2_->setText("Compressed Size");
        statTitle3_->setText("Compression Ratio");
        statTitle4_->setText("Space Saved");
    } else {
        statTitle1_->setText("Archive Size (.huf)");
        statTitle2_->setText("Restored Size");
        statTitle3_->setText("Decoded Bits");
        statTitle4_->setText("Restoration Status");
    }

    updateSuggestedOutput();
    updateActionState();
}

void MainWindow::onBrowseOutputDirClicked() {
    const QString initialDir = outputDirEdit_->text().isEmpty() ? QDir::homePath() : outputDirEdit_->text();
    const QString dir = QFileDialog::getExistingDirectory(this, "Select Output Folder", initialDir);
    if (!dir.isEmpty()) {
        outputDirEdit_->setText(dir);
    }
}

void MainWindow::updateSuggestedOutput() {
    if (selectedFilePath_.isEmpty()) {
        return;
    }

    QFileInfo info(selectedFilePath_);
    const bool isCompress = compressRadio_->isChecked();

    if (isCompress) {
        // e.g. report.txt -> report.txt.huf
        outputFileNameEdit_->setText(info.fileName() + ".huf");
    } else {
        // If input ends with .huf, strip .huf; if that conflicts with existing input, prefix "restored_"
        QString baseName = info.fileName();
        if (baseName.endsWith(".huf", Qt::CaseInsensitive)) {
            baseName.chop(4);
        }
        if (baseName.isEmpty()) {
            baseName = "decompressed_output.bin";
        }
        QFileInfo candidate(QDir(outputDirEdit_->text()).filePath(baseName));
        if (candidate.absoluteFilePath() == info.absoluteFilePath() || candidate.exists()) {
            baseName = "restored_" + baseName;
        }
        outputFileNameEdit_->setText(baseName);
    }
}

void MainWindow::onOutputFieldsEdited() {
    updateActionState();
}

void MainWindow::updateActionState() {
    const bool isCompress = compressRadio_->isChecked();
    actionBtn_->setText(isCompress ? "Compress File" : "Decompress Archive");

    const QString outDir = outputDirEdit_->text().trimmed();
    const QString outName = outputFileNameEdit_->text().trimmed();

    if (!outDir.isEmpty() && !outName.isEmpty()) {
        const QString fullOut = QDir(outDir).filePath(outName);
        fullOutputPreviewLabel_->setText("Destination: " + QDir::toNativeSeparators(fullOut));
    } else {
        fullOutputPreviewLabel_->setText("Destination: —");
    }

    const bool validInputs = !selectedFilePath_.isEmpty() &&
                             QFileInfo::exists(selectedFilePath_) &&
                             !outDir.isEmpty() &&
                             QFileInfo(outDir).isDir() &&
                             !outName.isEmpty() &&
                             !isProcessing_;

    actionBtn_->setEnabled(validInputs);
}

void MainWindow::setControlsEnabledForProcessing(bool processing) {
    isProcessing_ = processing;
    dropZone_->setEnabled(!processing);
    browseFileBtn_->setEnabled(!processing);
    clearFileBtn_->setEnabled(!processing && !selectedFilePath_.isEmpty());
    compressRadio_->setEnabled(!processing);
    decompressRadio_->setEnabled(!processing);
    outputDirEdit_->setEnabled(!processing);
    browseOutputDirBtn_->setEnabled(!processing);
    outputFileNameEdit_->setEnabled(!processing);
    progressBar_->setVisible(processing);

    if (processing) {
        actionBtn_->setEnabled(false);
        actionBtn_->setText(compressRadio_->isChecked() ? "Compressing..." : "Decompressing...");
        statusBanner_->setProperty("bannerState", "idle");
        style()->unpolish(statusBanner_);
        style()->polish(statusBanner_);
        statusBannerTitle_->setText("Processing in background...");
        statusBannerDetail_->setText("Running lossless Huffman engine without blocking the interface.");
        openFolderBtn_->setVisible(false);
        resetAnotherBtn_->setVisible(false);
    } else {
        updateActionState();
    }
}

void MainWindow::onRunActionClicked() {
    if (isProcessing_) {
        return;
    }

    const QString inPath = selectedFilePath_;
    const QString outDir = outputDirEdit_->text().trimmed();
    const QString outName = outputFileNameEdit_->text().trimmed();
    const bool isCompress = compressRadio_->isChecked();

    if (inPath.isEmpty() || !QFileInfo::exists(inPath) || !QFileInfo(inPath).isFile()) {
        statusBanner_->setProperty("bannerState", "error");
        style()->unpolish(statusBanner_);
        style()->polish(statusBanner_);
        statusBannerTitle_->setText("Input File Not Found");
        statusBannerDetail_->setText("Please select a valid existing file before starting.");
        return;
    }

    if (outDir.isEmpty() || !QFileInfo(outDir).isDir() || outName.isEmpty()) {
        statusBanner_->setProperty("bannerState", "error");
        style()->unpolish(statusBanner_);
        style()->polish(statusBanner_);
        statusBannerTitle_->setText("Invalid Output Destination");
        statusBannerDetail_->setText("Please select an existing output folder and specify a valid output filename.");
        return;
    }

    const QString fullOutPath = QDir(outDir).filePath(outName);
    if (QFileInfo(fullOutPath).absoluteFilePath() == QFileInfo(inPath).absoluteFilePath()) {
        statusBanner_->setProperty("bannerState", "error");
        style()->unpolish(statusBanner_);
        style()->polish(statusBanner_);
        statusBannerTitle_->setText("Output Conflict");
        statusBannerDetail_->setText("The output file path cannot be identical to the input file path.");
        return;
    }

    setControlsEnabledForProcessing(true);

    // Run FileCompressor or FileDecompressor asynchronously on a worker thread
    QFuture<OperationResult> future = QtConcurrent::run([inPath, fullOutPath, isCompress]() -> OperationResult {
        OperationResult res;
        res.isCompressMode = isCompress;
        res.inputFilePath = inPath;
        res.outputFilePath = fullOutPath;

        try {
            if (isCompress) {
                FileCompressor compressor;
                res.compressionStats = compressor.compressFile(
                    inPath.toStdString(),
                    fullOutPath.toStdString()
                );
            } else {
                FileDecompressor decompressor;
                res.decompressionStats = decompressor.decompressFile(
                    inPath.toStdString(),
                    fullOutPath.toStdString()
                );
            }
            res.succeeded = true;
        } catch (const std::exception& ex) {
            res.succeeded = false;
            const QString rawMsg = QString::fromUtf8(ex.what());
            if (rawMsg.contains("magic signature", Qt::CaseInsensitive) ||
                rawMsg.contains("smaller than header", Qt::CaseInsensitive)) {
                res.errorMessage = "The selected file is not a valid FileZipper (.huf) archive.";
            } else if (rawMsg.contains("Truncated", Qt::CaseInsensitive) ||
                       rawMsg.contains("Corrupted", Qt::CaseInsensitive)) {
                res.errorMessage = "The compressed archive appears to be damaged or incomplete and could not be restored.";
            } else if (rawMsg.contains("Failed to open", Qt::CaseInsensitive)) {
                res.errorMessage = "Could not open the input or output file. Please check file permissions and available disk space.";
            } else {
                res.errorMessage = "Operation could not be completed: " + rawMsg;
            }
        } catch (...) {
            res.succeeded = false;
            res.errorMessage = "An unexpected error occurred while processing the file.";
        }
        return res;
    });

    taskWatcher_->setFuture(future);
}

void MainWindow::onBackgroundTaskFinished() {
    lastResult_ = taskWatcher_->result();
    setControlsEnabledForProcessing(false);
    displayResultStats(lastResult_);
}

void MainWindow::displayResultStats(const OperationResult& result) {
    if (!result.succeeded) {
        statusBanner_->setProperty("bannerState", "error");
        style()->unpolish(statusBanner_);
        style()->polish(statusBanner_);
        statusBannerTitle_->setText(result.isCompressMode ? "Compression Failed" : "Decompression Failed");
        statusBannerDetail_->setText(result.errorMessage);
        openFolderBtn_->setVisible(false);
        resetAnotherBtn_->setVisible(true);
        return;
    }

    if (result.isCompressMode) {
        const CompressionStats& s = result.compressionStats;
        statTitle1_->setText("Original Size");
        statValue1_->setText(formatByteSize(s.originalFileSize));

        statTitle2_->setText("Compressed Size");
        statValue2_->setText(formatByteSize(s.compressedFileSize));

        statTitle3_->setText("Compression Ratio");
        if (s.originalFileSize == 0) {
            statValue3_->setText("0.00% (Empty File)");
            statValue4_->setText("0.00%");
        } else {
            statValue3_->setText(QString("%1%").arg(s.compressionRatio * 100.0, 0, 'f', 2));
            if (s.spaceSavedPercent >= 0.0) {
                statValue4_->setText(QString("%1%").arg(s.spaceSavedPercent, 0, 'f', 2));
            } else {
                statValue4_->setText(QString("0.00% (+%1 B header)").arg(s.compressedFileSize - s.originalFileSize));
            }
        }
        statTitle4_->setText("Space Saved");
    } else {
        const DecompressionStats& s = result.decompressionStats;
        statTitle1_->setText("Archive Size (.huf)");
        statValue1_->setText(formatByteSize(s.compressedFileSize));

        statTitle2_->setText("Restored Size");
        statValue2_->setText(formatByteSize(s.restoredFileSize));

        statTitle3_->setText("Decoded Bits");
        statValue3_->setText(QString("%1 bits").arg(s.decodedBitCount));

        statTitle4_->setText("Restoration Status");
        statValue4_->setText("Verified Lossless");
    }

    statusBanner_->setProperty("bannerState", "success");
    style()->unpolish(statusBanner_);
    style()->polish(statusBanner_);
    statusBannerTitle_->setText(
        result.isCompressMode
            ? "Compression Completed Successfully!"
            : "Decompression Completed Successfully!"
    );
    statusBannerDetail_->setText("Saved output to: " + QDir::toNativeSeparators(result.outputFilePath));
    openFolderBtn_->setVisible(true);
    resetAnotherBtn_->setVisible(true);
}

void MainWindow::onOpenOutputFolderClicked() {
    if (lastResult_.outputFilePath.isEmpty()) {
        return;
    }
    const QFileInfo outInfo(lastResult_.outputFilePath);
    QDesktopServices::openUrl(QUrl::fromLocalFile(outInfo.absolutePath()));
}

void MainWindow::onResetForAnotherClicked() {
    onClearInputClicked();
}

