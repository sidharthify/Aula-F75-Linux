#pragma once

#include <QByteArray>
#include <QWidget>

class QLabel;
class QPushButton;

namespace f75 {

class MainWindow;
class KeyboardView;

// Key matrix backup / restore and key swapping.
//
// The driver stores the key map as a matrix of 4-byte "hardware codes".  This
// page reads that matrix from the board, lets you swap two keys, and writes it
// back.  Reading and writing the matrix has not yet been verified against the
// physical board, so the page is marked experimental and always keeps a backup
// before writing.
class RemapPage : public QWidget
{
    Q_OBJECT
public:
    explicit RemapPage(MainWindow* mw, QWidget* parent = nullptr);

private slots:
    void readMatrix();
    void writeMatrix();
    void backup();
    void restore();
    void swapKeys();
    void resetMatrix();
    void onKeyClicked(int led);

private:
    void setStatus(const QString& s);
    void updateView();

    MainWindow*   m_mw = nullptr;
    KeyboardView* m_view = nullptr;
    QLabel*       m_status = nullptr;
    QLabel*       m_sourceLabel = nullptr;
    QLabel*       m_targetLabel = nullptr;
    QPushButton*  m_writeBtn = nullptr;

    QByteArray m_matrix;   // raw layer-0 matrix (4 bytes per position)
    int        m_source = -1;
    int        m_target = -1;
    bool       m_haveBackup = false;
};

} // namespace f75
