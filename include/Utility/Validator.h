#pragma once
#include <QLineEdit>
#include <QValidator>

namespace Validators
{
inline void setIpLineEdit(QLineEdit* edit)
{
	// 限制输入框输入IP地址
	static const auto validator = [] {
		auto re = QString("^(%1\\.){3}(%1)$").arg("2[0-4]\\d|25[0-5]|[01]?\\d\\d?");
		QRegularExpression reg(re);
		return QRegularExpressionValidator(reg);
	} ();
	edit->setValidator(&validator);
	edit->setInputMask("000.000.000.000;_");
}

inline void setPortLineEdit(QLineEdit* edit)
{
	static const QIntValidator validator(0, 0xFFFF);
	edit->setValidator(&validator);
}
}
