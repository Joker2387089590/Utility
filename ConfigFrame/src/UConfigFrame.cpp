#include <QListWidgetItem>
#include <QFileDialog>
#include <QCloseEvent>

#include "UConfigFrame.h"
#include "UConfigWidget.h"
#include "UConfigWidgetData.h"
#include "ui_UConfigFrame.h"

// 调用所有 Config Widget 的成员函数 func
void UConfigFrame::CallAll(void (UConfigWidget::*func)())
{
	for(auto&& [className, config] : m_configs) (config->*func)();
}

UConfigFrame::UConfigFrame(QWidget* parent) :
	QWidget(parent),
	ui(new Ui::UConfigFrame)
{
	ui->setupUi(this);

	auto ResetAll	= [this]() { CallAll(&UConfigWidget::Reset); };
	auto ApplyAll	= [this]() { CallAll(&UConfigWidget::Apply); };
	auto CancelAll	= [this]() { CallAll(&UConfigWidget::Cancel); close(); };
	auto ConfirmAll	= [this, ApplyAll]() { ApplyAll(); close(); };

	connect(ui->btnOk,		&QPushButton::clicked,		ConfirmAll);
	connect(ui->btnCancel,	&QPushButton::clicked,		CancelAll);
	connect(ui->btnApply,	&QPushButton::clicked,		ApplyAll);
	connect(this,			&UConfigFrame::doResetAll,	ResetAll);
	connect(ui->classList,	&QListWidget::currentItemChanged,
			this,			&UConfigFrame::ChangeWidget);
}

UConfigFrame::~UConfigFrame() { ins::detach(); delete ui; }

void UConfigFrame::AddWidget(UConfigWidget* config)
{
	if (!config || (m_configs.find(config->ClassName()) != m_configs.end()))
		return;

	m_configs.insert({ config->ClassName(), config });
	ui->configs->addWidget(config);

	connect(config, &UConfigWidget::modifyStateChanged,
			[this](bool) { ui->btnApply->setEnabled(IsModified()); });

	// TODO: Add icon
	auto item = new QListWidgetItem(config->Title(), ui->classList);
	item->setData(Qt::UserRole, config->ClassName());
}

void UConfigFrame::ChangeWidget(QListWidgetItem* item)
{
	auto config = m_configs[item->data(Qt::UserRole).toString()];
	ui->configs->setCurrentWidget(config);
	ui->title->setText(config->Title());
}

void UConfigFrame::on_btnExport_clicked()
{
	auto fileName = QFileDialog::getSaveFileName(this, tr("Export config file"));
	if(fileName.isEmpty()) return; // 按了取消
	currentConfig()->d->SaveHelper(fileName, &UConfigWidget::SaveData);
}

void UConfigFrame::on_btnExportAll_clicked()
{
	auto dirName = QFileDialog::getExistingDirectory(this, tr("Export all config files"));
	if(dirName.isEmpty()) return;
	for(auto&& [className, config] : m_configs)
	{
		auto fileName = dirName + '/' + config->Title();
		config->d->SaveHelper(fileName, &UConfigWidget::SaveData);
	}
}

void UConfigFrame::on_btnImport_clicked()
{
	auto fileNames = QFileDialog::getOpenFileNames(this);
	if(fileNames.isEmpty()) return;

	for(auto&& fileName : fileNames)
	{
		QFile file(fileName);
		if(!file.open(QFile::ReadOnly))
		{
			// TODO: Add error handle
			return;
		}

		QDataStream stream(&file);
		auto pos = m_configs.find(GetFileClass(stream));
		if(pos != m_configs.end())
			pos->second->LoadData(stream);
		else
		{
			// TODO: Add error handle
		}
	}
}

void UConfigFrame::on_btnReset_clicked()
{
	auto config = currentConfig();
	auto result = QMessageBox::warning(this,
		tr("Reset config"),
		tr("Trying to reset %1 to default, going on?").arg(config->Title()),
		QMessageBox::Yes | QMessageBox::Cancel);
	if(result == QMessageBox::Yes) config->Reset();
}

void UConfigFrame::on_btnResetAll_clicked()
{
	auto result = QMessageBox::warning(
		this, tr("Reset config"),
		tr("Trying to reset all config to default, going on?"),
		QMessageBox::Yes | QMessageBox::Cancel);
	if(result == QMessageBox::Yes) emit doResetAll();
}

UConfigWidget* UConfigFrame::currentConfig() const
{
	return qobject_cast<UConfigWidget*>(ui->configs->currentWidget());
}

bool UConfigFrame::IsModified() const
{
	for(auto&& [className, config] : m_configs)
		if(config->IsModified())
			return true;
	return false;
}

void UConfigFrame::closeEvent(QCloseEvent* event)
{
	bool needClose = true;
	if(IsModified())
	{
		QMessageBox msgBox;
		msgBox.setText(tr("The configs has been modified."));
		msgBox.setInformativeText(tr("Do you want to save your changes?"));
		auto buttons = QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel;
		msgBox.setStandardButtons(buttons);

		switch(msgBox.exec())
		{
		case QMessageBox::Save:
			ui->btnOk->click();
			break;
		case QMessageBox::Discard:
			ui->btnCancel->click();
			break;
		case QMessageBox::Cancel:
		default:
			needClose = false;
			break;
		}
	}
	event->setAccepted(needClose);
}
