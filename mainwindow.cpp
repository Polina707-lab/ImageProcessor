#include "MainWindow.h"
#include "ImageProcessor.h"

#include <QAction>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QSpinBox>
#include <QComboBox>
#include <QPushButton>
#include <QScrollArea>
#include <QStatusBar>
#include <QMessageBox>
#include <QPixmap>
#include <QMenuBar>
#include <QFrame>
#include <QFormLayout>
#include <QGuiApplication>
#include <QScreen>

static QLabel* makeImageLabel() {
    auto* lab = new QLabel;
    lab->setBackgroundRole(QPalette::Base);
    lab->setAlignment(Qt::AlignCenter);
    lab->setMinimumSize(300, 300);
    lab->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    return lab;
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    buildUi();
    statusBar()->showMessage("Откройте изображение (Файл → Открыть)...");
}

    void MainWindow::buildUi()
{
    // --- Меню ---
    QMenu* menuFile = menuBar()->addMenu("Файл");
    QAction* actOpen  = menuFile->addAction("Открыть...");
    QAction* actSave  = menuFile->addAction("Сохранить...");
    QAction* actReset = menuFile->addAction("Сбросить изображение");

    connect(actOpen,  &QAction::triggered, this, &MainWindow::openImage);
    connect(actSave,  &QAction::triggered, this, &MainWindow::saveImage);
    connect(actReset, &QAction::triggered, this, &MainWindow::resetImage);

    // --- Изображения ---
    viewOrig_ = makeImageLabel();
    viewProc_ = makeImageLabel();

    viewOrig_->setAlignment(Qt::AlignCenter);
    viewProc_->setAlignment(Qt::AlignCenter);
    viewOrig_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewProc_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    viewOrig_->setMinimumSize(200, 200);
    viewProc_->setMinimumSize(200, 200);

    auto* leftArea = new QScrollArea;
    leftArea->setWidget(viewOrig_);
    leftArea->setWidgetResizable(true);
    leftArea->setFrameShape(QFrame::StyledPanel);

    auto* rightArea = new QScrollArea;
    rightArea->setWidget(viewProc_);
    rightArea->setWidgetResizable(true);
    rightArea->setFrameShape(QFrame::StyledPanel);

    QLabel* labelOrig = new QLabel("Оригинал");
    QLabel* labelProc = new QLabel("Результат");
    labelOrig->setObjectName("imageTitle");
    labelProc->setObjectName("imageTitle");

    labelOrig->setAlignment(Qt::AlignCenter);
    labelProc->setAlignment(Qt::AlignCenter);

    QVBoxLayout* leftLayout = new QVBoxLayout;
    leftLayout->addWidget(leftArea, 1);
    leftLayout->addWidget(labelOrig);

    QVBoxLayout* rightLayout = new QVBoxLayout;
    rightLayout->addWidget(rightArea, 1);
    rightLayout->addWidget(labelProc);

    QHBoxLayout* imagesLayout = new QHBoxLayout;
    imagesLayout->setSpacing(20);
    imagesLayout->setContentsMargins(40, 10, 40, 10);
    imagesLayout->addLayout(leftLayout, 1);
    imagesLayout->addLayout(rightLayout, 1);

    QWidget* imagesWidget = new QWidget;
    imagesWidget->setLayout(imagesLayout);

    // --- Панель управления ---
    comboOp_ = new QComboBox;
    comboOp_->addItems({
        "Глобальная пороговая обработка",
        "Адаптивная пороговая обработка",
        "Обнаружение границ (Собель)",
        "Обнаружение линий (ядра)",
        "Обнаружение точек (Лаплас)"
    });

    comboGlobal_ = new QComboBox;
    comboGlobal_->addItems({ "Метод Отсу", "Метод среднего значения" });

    comboAdapt_ = new QComboBox;
    comboAdapt_->addItems({ "α-метод" });

    winSize_ = new QSpinBox;
    winSize_->setRange(3, 99);
    winSize_->setSingleStep(2);
    winSize_->setValue(25);

    // Создаём поле для α
    alphaSpin_ = new QDoubleSpinBox;
    alphaSpin_->setRange(0.3, 0.8);       // диапазон
    alphaSpin_->setSingleStep(0.05);      // шаг изменения
    alphaSpin_->setDecimals(2);           // два знака после запятой
    alphaSpin_->setValue(2.0 / 3.0);      // значение по умолчанию


    sobelTh_ = new QSpinBox;
    sobelTh_->setRange(0, 4096);
    sobelTh_->setValue(100);

    lineTh_  = new QSpinBox;
    lineTh_->setRange(0, 4096);
    lineTh_->setValue(120);

    pointTh_ = new QSpinBox;
    pointTh_->setRange(0, 4096);
    pointTh_->setValue(150);

    // --- Кнопка ---
    btnApply_ = new QPushButton("ПРИМЕНИТЬ");
    btnApply_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnApply_->setFixedHeight(45);
    connect(btnApply_, &QPushButton::clicked, this, &MainWindow::applyOperation);

    // --- Компоновка элементов управления ---
    QGridLayout* controls = new QGridLayout;
    controls->setHorizontalSpacing(25);
    controls->setVerticalSpacing(10);
    controls->setAlignment(Qt::AlignCenter);

    int r = 0;


    // --- Блок 1: операция ---
    controls->addWidget(new QLabel("Операция:"), r, 0, Qt::AlignRight);
    controls->addWidget(comboOp_, r, 1, 1, 3);
    r++;

    // --- Разделитель ---
    {
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        controls->addWidget(line, r, 0, 1, 4);
        r++;
    }

    // --- Блок 2: глобальный метод ---
    controls->addWidget(new QLabel("Глобальный метод:"), r, 0, Qt::AlignRight);
    controls->addWidget(comboGlobal_, r, 1, 1, 3);
    r++;

    // --- Разделитель ---
    {
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        controls->addWidget(line, r, 0, 1, 4);
        r++;
    }

    // --- Блок 3: адаптивный метод ---
    QHBoxLayout* alphaLayout = new QHBoxLayout;
    alphaLayout->setSpacing(8);
    alphaLayout->addWidget(new QLabel("α:"));
    alphaLayout->addWidget(alphaSpin_);
    alphaLayout->addWidget(new QLabel("Размер окна:"));
    alphaLayout->addWidget(winSize_);
    alphaLayout->addStretch();

    controls->addWidget(new QLabel("Адаптивный метод:"), r, 0, Qt::AlignRight);
    controls->addLayout(alphaLayout, r, 1, 1, 4); // растянем на несколько колонок
    r++;



    // --- Разделитель ---
    {
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        controls->addWidget(line, r, 0, 1, 5);
        r++;
    }

    // --- Блок 4: пороги ---
    QHBoxLayout* threshLayout = new QHBoxLayout;
    threshLayout->setSpacing(3); // расстояние между блоками

    // Собель
    threshLayout->addWidget(new QLabel("Порог Собеля:"));
    threshLayout->addWidget(sobelTh_);

    // Линии
    threshLayout->addSpacing(6);
    threshLayout->addWidget(new QLabel("Порог линий:"));
    threshLayout->addWidget(lineTh_);

    // Точки
    threshLayout->addSpacing(10);
    threshLayout->addWidget(new QLabel("Порог точек:"));
    threshLayout->addWidget(pointTh_);


    controls->addLayout(threshLayout, r, 0, 1, 5);
    r++;


    // --- Разделитель перед кнопкой ---
    {
        QFrame* line = new QFrame;
        line->setFrameShape(QFrame::HLine);
        line->setFrameShadow(QFrame::Sunken);
        controls->addWidget(line, r, 0, 1, 5);
        r++;
    }

    // --- Общий layout для панели ---
    QVBoxLayout* controlsLayout = new QVBoxLayout;
    controlsLayout->addLayout(controls);
    controlsLayout->addSpacing(20);
    controlsLayout->addWidget(btnApply_);
    controlsLayout->setContentsMargins(80, 35, 80, 20);

    QWidget* controlsWidget = new QWidget;
    controlsWidget->setObjectName("controlsWidget"); // для стилей QSS
    controlsWidget->setLayout(controlsLayout);

    // --- Общая структура окна ---
    QVBoxLayout* mainLayout = new QVBoxLayout;
    mainLayout->addWidget(imagesWidget, 6);  // ↑ делаем блок изображений выше
    mainLayout->addWidget(controlsWidget, 0); // ↓ панель управления компактнее

    mainLayout->setContentsMargins(20, 10, 20, 10);

    QWidget* central = new QWidget;
    central->setLayout(mainLayout);
    setCentralWidget(central);

    resize(950, 870);

    // Центрируем окно
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int x = (screenGeometry.width() - width()) / 2;
    int y = (screenGeometry.height() - height()) / 2- 13;
    move(x, y);

    setFixedSize(size());
}




void MainWindow::openImage()
{
    QString fn = QFileDialog::getOpenFileName(this, "Открыть изображение", {}, "Изображения (*.png *.jpg *.bmp)");
    if (fn.isEmpty()) return;
    QImage img(fn);
    if (img.isNull()) {
        QMessageBox::warning(this, "Ошибка", "Не удалось загрузить изображение");
        return;
    }

    original_ = Img::toGrayscale(img);
    current_  = original_;
    updatePreview(current_);
    viewOrig_->setPixmap(QPixmap::fromImage(original_).scaled(
        viewOrig_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    statusBar()->showMessage("Изображение загружено: " + fn);
}

void MainWindow::saveImage()
{
    if (current_.isNull()) { QMessageBox::information(this, "Сохранение", "Нет обработанного изображения"); return; }
    QString fn = QFileDialog::getSaveFileName(this, "Сохранить изображение", "результат.png", "PNG Image (*.png)");
    if (fn.isEmpty()) return;
    current_.save(fn);
    statusBar()->showMessage("Сохранено: " + fn);
}

void MainWindow::resetImage()
{
    if (original_.isNull()) return;
    current_ = original_;
    updatePreview(current_);
    statusBar()->showMessage("Изображение сброшено");
}

void MainWindow::applyOperation()
{
    if (original_.isNull()) {
        QMessageBox::information(this, "Информация", "Сначала откройте изображение.");
        return;
    }

    const QImage gray = original_;
    QImage out;
    const int op = comboOp_->currentIndex();

    // --- Глобальная пороговая обработка ---
    if (op == 0) {
        switch (comboGlobal_->currentIndex()) {
        case 0:
            out = Img::thresholdOtsu(gray);
            statusBar()->showMessage("Глобальная пороговая обработка (метод Отсу)");
            break;
        case 1:
            out = Img::thresholdMean(gray);
            statusBar()->showMessage("Глобальная пороговая обработка (метод среднего значения)");
            break;
        }

        // --- Адаптивная пороговая обработка ---
    } else if (op == 1) {
    out = Img::adaptiveAlpha(gray, winSize_->value(), alphaSpin_->value());
    statusBar()->showMessage(QString("Адаптивная пороговая обработка (α = %1)").arg(alphaSpin_->value(), 0, 'f', 2));


        // --- Обнаружение границ ---
    } else if (op == 2) {
        out = Img::edgesSobel(gray, sobelTh_->value());
        statusBar()->showMessage("Обнаружение границ (оператор Собеля)");

        // --- Обнаружение линий ---
    } else if (op == 3) {
        out = Img::linesKernels(gray, lineTh_->value());
        statusBar()->showMessage("Обнаружение линий");

        // --- Обнаружение точек ---
    } else if (op == 4) {
        out = Img::pointsLaplacian(gray, pointTh_->value());
        statusBar()->showMessage("Обнаружение точек (оператор Лапласа)");
    }

    current_ = out;
    updatePreview(current_);
}



void MainWindow::updatePreview(const QImage& img)
{
    viewProc_->setPixmap(QPixmap::fromImage(img).scaled(
        viewProc_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}


void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);

    if (!original_.isNull())
        viewOrig_->setPixmap(QPixmap::fromImage(original_).scaled(
            viewOrig_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));

    if (!current_.isNull())
        viewProc_->setPixmap(QPixmap::fromImage(current_).scaled(
            viewProc_->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

