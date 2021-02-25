#include <QApplication>
#include <QDir>
#include "UConfigWidget.h"
#include "UConfigWidgetData.h"
#include "UConfigFrame.h"

UConfigWidget::UConfigWidget(const QString& className,
							 const QString& suffix,
							 const QString& title) :
	QWidget(nullptr, Qt::Window), // UConfigFrame::AddWidget 会管理这个类的生命周期
	d(new UConfigWidgetData{this, className, suffix,
							title.isEmpty() ? className : title,
							false})
{
	UConfigFrame::ins()->AddWidget(this);
}

UConfigWidget::~UConfigWidget()
{
}

void UConfigWidget::Apply()
{
	if(!IsModified()) return;
	ApplyConfig();
	SetModified(false);
	emit applied();
}

void UConfigWidget::Cancel()
{
	DiscardConfig();
	SetModified(false);
}

void UConfigWidget::Reset()
{
	d->LoadHelper(DefaultFile(), &UConfigWidget::ResetConfig);
	Modify();
}

QString UConfigWidget::ConfigDir()
{
	QDir dir(qApp->applicationFilePath() + "/config");
	bool makeDefault = dir.mkpath("default");
	bool makeCurrent = dir.mkpath("current");
	if(!makeDefault || !makeCurrent)
	{
		// TODO: error handle
	}
	return dir.absolutePath();
}

bool UConfigWidgetData::LoadHelper(const QString& fileName,
								   bool (UConfigWidget::*work)(QDataStream&))
{
	// 打开文件
	QFile file(fileName);
	if(!file.open(QFile::ReadOnly)) return false;

	// 检查是否是这个类的文件
	QDataStream stream(&file);
	if(GetFileClass(stream) != className) return false;

	// 执行加载操作
	return (w->*work)(stream);
}

bool UConfigWidgetData::SaveHelper(const QString& fileName,
								   bool (UConfigWidget::*work)(QDataStream&) const) const
{
	// 打开文件
	QFile file(fileName);
	if(!file.open(QFile::WriteOnly)) return false;

	// 保存类名
	QDataStream stream(&file);
	SetFileClass(stream, w);

	// 执行保存操作
	return (w->*work)(stream);
}

std::pair<QString, bool> UConfigWidgetData::PathHelper(const QString& subdir) const
{
	QFileInfo fileInfo(w->ConfigDir() + subdir + className + '.' + suffix);
	return { fileInfo.absoluteFilePath(), fileInfo.exists() };
}

QString UConfigWidget::DefaultFile() const
{
	auto&& [path, exist] = d->PathHelper("/default/");

	// 创建只包含 className 的文件
	if(!exist && !d->SaveHelper(path, &UConfigWidget::SaveNothing))
	{
		// TODO: error handle
	}
	return path;
}

QString UConfigWidget::CurrentFile() const
{
	auto&& [path, exist] = d->PathHelper("/current/");

	// 复制默认文件
	if(!exist && !QFile::copy(DefaultFile(), path))
	{
		// TODO: error handle
		throw std::exception(path.toUtf8());
	}
	return path;
}

void UConfigWidget::LoadCurrent()
{
	d->LoadHelper(CurrentFile(), &UConfigWidget::LoadData);
}
void UConfigWidget::SaveCurrent() const
{
	d->SaveHelper(CurrentFile(), &UConfigWidget::SaveData);
}

const QString &UConfigWidget::ClassName() const { return d->className; }
const QString &UConfigWidget::Title() const { return d->title; }
void UConfigWidget::SetTitle(const QString &title) { d->title = title; }

bool UConfigWidget::IsModified() const { return d->hasModified; }
void UConfigWidget::SetModified(bool isModified)
{
	emit modifyStateChanged((d->hasModified = isModified));
}
