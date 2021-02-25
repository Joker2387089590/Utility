#pragma once
#include <QString>
#include <QMessageBox>
#include <QFile>
#include <QFileInfo>

class UConfigWidget;
struct UConfigWidgetData
{
	UConfigWidget* w;
	const QString className;	// 子类的类名，用于 Config Base 文件命名
	const QString suffix;		// 文件后缀名
	QString title;				// 翻译过的窗口标题
	bool hasModified;			// 修改标记

	std::pair<QString, bool> PathHelper(const QString &subdir) const;
	bool LoadHelper(const QString& fileName,
					bool (UConfigWidget::*work)(QDataStream&));
	bool SaveHelper(const QString& fileName,
					bool (UConfigWidget::*work)(QDataStream&) const) const;
};
