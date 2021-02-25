#pragma once
#include <QJsonDocument>
#include <QJsonObject>
#include "UConfigWidget.h"

class CONFIGFRAME_EXPORT UJsonConfig : public UConfigWidget
{
	Q_OBJECT
public:
	explicit UJsonConfig(const QString& className, const QString& title = {});

protected:
	void ApplyConfig() override;
	void DiscardConfig() override;
	bool ResetConfig(QDataStream& stream) override;
	bool LoadData(QDataStream& stream) override;
	bool SaveData(QDataStream& stream) const override;

	QVariantMap m_useData;
	QVariantMap m_showData;
};
