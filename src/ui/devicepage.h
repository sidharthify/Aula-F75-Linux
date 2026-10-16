#pragma once

#include <QWidget>

class QLabel;

namespace f75 {

// Read-only device information page.
class DevicePage : public QWidget
{
    Q_OBJECT
public:
    explicit DevicePage(QWidget* parent = nullptr);

    void setConnection(bool connected, const QString& path);
    void setModel(const QString& model, int modelId);
    void setFirmware(const QString& fw);
    void setId(uint16_t vid, uint16_t pid);
    void setStatus(const QString& text);

private:
    QLabel* m_conn = nullptr;
    QLabel* m_model = nullptr;
    QLabel* m_fw = nullptr;
    QLabel* m_id = nullptr;
    QLabel* m_status = nullptr;
};

} // namespace f75
