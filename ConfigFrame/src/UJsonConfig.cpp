#include "UJsonConfig.h"

UJsonConfig::UJsonConfig(const QString &className, const QString &title) :
	UConfigWidget(className, "json", title)
{
	LoadCurrent();
}

void UJsonConfig::ApplyConfig() { m_useData = m_showData; }

void UJsonConfig::DiscardConfig() { m_showData = m_useData; }

bool UJsonConfig::LoadData(QDataStream &stream)
{
	QByteArray jsonText;
	stream >> jsonText;

	auto json = QJsonDocument::fromJson(jsonText);
	if(!json.isObject()) return false;

	m_useData = json.object().toVariantMap();
	return true;
}

bool UJsonConfig::SaveData(QDataStream &stream) const
{
	auto jsonText = QJsonDocument(QJsonObject::fromVariantMap(m_useData)).toJson();
	stream << jsonText;
	return true;
}

bool UJsonConfig::ResetConfig(QDataStream& stream)
{
	std::swap(m_useData, m_showData);
	bool successLoad = LoadData(stream);
	std::swap(m_useData, m_showData);
	return successLoad;
}
