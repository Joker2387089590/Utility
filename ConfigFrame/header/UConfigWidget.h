#pragma once
#include <QWidget>
#include <QDataStream>
#include "UConfigFrame.h"

struct UConfigWidgetData;
class CONFIGFRAME_EXPORT UConfigWidget : public QWidget
{
	Q_OBJECT
public:
	explicit UConfigWidget(const QString& className,	// 子类的类名，不翻译
						   const QString& suffix,		// 子类的配置文件的后缀，不需要带点
						   const QString& title = {});	// 翻译过的类名
	~UConfigWidget() override;

	// 响应按钮
	void Apply();
	void Cancel();
	void Reset();

	// 数据是否修改了
	bool IsModified() const;
	void SetModified(bool isModified);
	inline void Modify() { SetModified(true); }

	// 配置保存的文件夹
	static QString ConfigDir();
	QString DefaultFile() const; // 默认文件，不存在时创建空文件
	QString CurrentFile() const; // 当前文件，不存在时复制默认文件

	// 从当前文件导入导出
	void LoadCurrent();
	void SaveCurrent() const;

	const QString& ClassName() const;
	const QString& Title() const;
	void SetTitle(const QString& title);

signals:
	void applied();
	void modifyStateChanged(bool isModified);

protected:
	// 修改配置数据的操作，不需要手动控制 modified
	virtual void ApplyConfig() = 0;
	virtual void DiscardConfig() = 0;
	virtual bool ResetConfig(QDataStream& stream) = 0;

	virtual bool LoadData(QDataStream& stream) = 0;
	virtual bool SaveData(QDataStream& stream) const = 0;

	// 什么都不保存
	inline bool SaveNothing(QDataStream&) const { return true; }

private:
	friend class UConfigFrame;
	friend struct UConfigWidgetData;
	std::unique_ptr<UConfigWidgetData> d;
};

inline QString GetFileClass(QDataStream& stream)
{
	QString className;
	stream >> className;
	return className;
}
inline void SetFileClass(QDataStream& stream, const UConfigWidget* config)
{
	stream << config->ClassName();
}
