#pragma once
#include <map>
#include <QWidget>
#include "Singleton.h"
#include "ConfigFrame_global.h"

namespace Ui { class UConfigFrame; }
class UConfigWidget;
class QListWidgetItem;

class CONFIGFRAME_EXPORT UConfigFrame : public QWidget
{
	Q_OBJECT
	SINGLETON
public:
	~UConfigFrame();

	void AddWidget(UConfigWidget* config);
	UConfigWidget* currentConfig() const;

	// 是否有数据已修改
	bool IsModified() const;

signals:
	void doResetAll();

protected:
	void closeEvent(QCloseEvent* event) override;

private:
	Ui::UConfigFrame* ui;
	std::map<QString, UConfigWidget*> m_configs; // class name -> config widget

	explicit UConfigFrame(QWidget* parent = nullptr);
	void ChangeWidget(QListWidgetItem* item);
	void CallAll(void (UConfigWidget::*func)());

private slots:
	void on_btnExport_clicked();
	void on_btnExportAll_clicked();
	void on_btnImport_clicked();
	void on_btnReset_clicked();
	void on_btnResetAll_clicked();
};

DECLARE_SINGLETON_INS(UConfigFrame);
