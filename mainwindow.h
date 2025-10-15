#pragma once
#include <QMainWindow>
#include <QImage>

class QLabel;
class QComboBox;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent=nullptr);

protected:
void resizeEvent(QResizeEvent* event) override;

private slots:
    void openImage();
    void saveImage();
    void resetImage();
    void applyOperation();

private:
    void updatePreview(const QImage& img);
    void buildUi();

    QImage original_;
    QImage current_;

    QLabel* viewOrig_;
    QLabel* viewProc_;

    QComboBox* comboOp_;     // выбор операции
    QComboBox* comboGlobal_; // Otsu/Iterative
    QComboBox* comboAdapt_;
    QDoubleSpinBox* alphaSpin_;     // список адаптивных
    QSpinBox*  winSize_;
    QSpinBox*  adaptC_;      // не используется в α-методе, но оставлен в UI (на будущее)
    QDoubleSpinBox* sigma_;  // "
    QSpinBox* sobelTh_;
    QSpinBox* lineTh_;
    QSpinBox* pointTh_;
    QPushButton* btnApply_;
};
