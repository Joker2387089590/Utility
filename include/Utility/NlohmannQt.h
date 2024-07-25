#pragma once
#include <nlohmann/json.hpp>
#include <QJsonObject>
#include <QJsonArray>

// Qt JSON 与 nlohmann-json 类型互相转换
// 参考 https://json.nlohmann.me/features/arbitrary_types/#how-do-i-convert-third-party-types

NLOHMANN_JSON_NAMESPACE_BEGIN

/// declare

template<>
struct adl_serializer<QString>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QString& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QString& s);
};

template<>
struct adl_serializer<QJsonObject>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QJsonObject& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QJsonObject& s);
};

template<>
struct adl_serializer<QJsonArray>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QJsonArray& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QJsonArray& s);
};

template<>
struct adl_serializer<QJsonValue>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QJsonValue& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QJsonValue& s);
};

template<typename T>
struct adl_serializer<QList<T>>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QList<T>& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QList<T>& s);
};

template<typename T>
struct adl_serializer<QMap<QString, T>>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QMap<QString, T>& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QMap<QString, T>& s);
};

template<typename T>
struct adl_serializer<QHash<QString, T>>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QHash<QString, T>& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QHash<QString, T>& s);
};

template<typename T>
struct adl_serializer<QMap<std::string, T>>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QMap<std::string, T>& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QMap<std::string, T>& s);
};

template<typename T>
struct adl_serializer<QHash<std::string, T>>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QHash<std::string, T>& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QHash<std::string, T>& s);
};

/// impl

template<>
struct adl_serializer<QVariant>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const QVariant& qv);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, QVariant& qv);
};

template<typename BasicJsonType>
void adl_serializer<QString>::to_json(BasicJsonType& j, const QString& s)
{
	j = s.toStdString();
}

template<typename BasicJsonType>
void adl_serializer<QString>::from_json(const BasicJsonType& j, QString& s)
{
	s = QString::fromStdString(j.template get<std::string>());
}

template<typename BasicJsonType>
void adl_serializer<QJsonObject>::to_json(BasicJsonType& j, const QJsonObject& s)
{
	j = BasicJsonType::object();
	auto b = s.begin();
	auto e = s.end();
	for(auto i = b; i != e; ++i)
		j[i.key().toStdString()] = QJsonValue(i.value());
}

template<typename BasicJsonType>
void adl_serializer<QJsonObject>::from_json(const BasicJsonType& j, QJsonObject& s)
{
	for(auto&& [key, value] : j.items())
		s.insert(QString::fromStdString(key), value);
}

template<typename BasicJsonType>
void adl_serializer<QJsonArray>::to_json(BasicJsonType& j, const QJsonArray& arr)
{
	j = BasicJsonType::array();
	for(auto&& v : arr) j.push_back(QJsonValue(v));
}

template<typename BasicJsonType>
void adl_serializer<QJsonArray>::from_json(const BasicJsonType& j, QJsonArray& arr)
{
	for(auto&& v : j) arr.push_back(v);
}

template<typename BasicJsonType>
void adl_serializer<QJsonValue>::to_json(BasicJsonType& j, const QJsonValue& value)
{
	switch(value.type())
	{
	case QJsonValue::Bool:   j = value.toBool();   break;
	case QJsonValue::Double: j = value.toDouble(); break;
	case QJsonValue::String: j = value.toString(); break;
	case QJsonValue::Array:  j = value.toArray();  break;
	case QJsonValue::Object: j = value.toObject(); break;
	case QJsonValue::Null:   j = {};               break;

	case QJsonValue::Undefined:
	default:
		break;
	}
}

template<typename BasicJsonType>
void adl_serializer<QJsonValue>::from_json(const BasicJsonType& j, QJsonValue& value)
{
	switch(j.type())
	{
	case BasicJsonType::value_t::null:            value = QJsonValue::Null;              break;
	case BasicJsonType::value_t::object:          value = j.template get<QJsonObject>(); break;
	case BasicJsonType::value_t::array:           value = j.template get<QJsonArray>();  break;
	case BasicJsonType::value_t::string:          value = j.template get<QString>();     break;
	case BasicJsonType::value_t::boolean:         value = j.template get<bool>();        break;
	case BasicJsonType::value_t::number_integer:  value = j.template get<qint64>();      break;
	case BasicJsonType::value_t::number_unsigned: value = j.template get<qint64>();      break;
	case BasicJsonType::value_t::number_float:    value = j.template get<double>();      break;

	case BasicJsonType::value_t::binary:
	case BasicJsonType::value_t::discarded:
	default:
		value = QJsonValue::Undefined;
		break;
	}
}

template<typename BasicJsonType>
void adl_serializer<QVariant>::to_json(BasicJsonType& j, const QVariant& qv)
{
	switch(qv.typeId())
	{
	case QMetaType::Bool: j = qv.value<bool>(); break;
	case QMetaType::Int: j = qv.value<int>(); break;
	case QMetaType::UInt: j = qv.value<unsigned int>(); break;
	case QMetaType::Double: j = qv.value<double>(); break;
	case QMetaType::QChar: j = qv.value<QChar>().unicode(); break;
	case QMetaType::QString: j = qv.value<QString>(); break;
	case QMetaType::QByteArray: j = qv.value<QByteArray>(); break;
	case QMetaType::Long: j = qv.value<long>(); break;
	case QMetaType::LongLong: j = qv.value<qlonglong>(); break;
	case QMetaType::Short: j = qv.value<short>(); break;
	case QMetaType::Char: j = qv.value<char>(); break;
	case QMetaType::Char16: j = qv.value<char16_t>(); break;
	case QMetaType::Char32: j = qv.value<char32_t>(); break;
	case QMetaType::ULong: j = qv.value<unsigned long>(); break;
	case QMetaType::ULongLong: j = qv.value<qulonglong>(); break;
	case QMetaType::UShort: j = qv.value<unsigned short>(); break;
	case QMetaType::SChar: j = qv.value<signed char>(); break;
	case QMetaType::UChar: j = qv.value<unsigned char>(); break;
	case QMetaType::Float: j = qv.value<float>(); break;
	case QMetaType::QVariantList: j = qv.value<QVariantList>(); break;
	case QMetaType::QStringList: j = qv.value<QStringList>(); break;
	case QMetaType::QVariantMap: j = qv.value<QVariantMap>(); break;
	case QMetaType::QVariantHash: j = qv.value<QVariantHash>(); break;
	case QMetaType::QVariantPair: j = qv.value<QVariantPair>(); break;
	case QMetaType::QJsonValue: j = qv.value<QJsonValue>(); break;
	case QMetaType::QJsonObject: j = qv.value<QJsonObject>(); break;
	case QMetaType::QJsonArray: j = qv.value<QJsonArray>(); break;
	case QMetaType::QByteArrayList: j = qv.value<QByteArrayList>(); break;
	default: j = {}; break;
	}
}

template<typename BasicJsonType>
void adl_serializer<QVariant>::from_json(const BasicJsonType& j, QVariant& qv)
{
	auto type = qv.metaType();
	if(!type.isValid())
	{
		switch(j.type())
		{
		case BasicJsonType::value_t::null:            qv = {};                              break;
		case BasicJsonType::value_t::object:          qv = j.template get<QJsonObject>();   break;
		case BasicJsonType::value_t::array:           qv = j.template get<QJsonArray>();    break;
		case BasicJsonType::value_t::string:          qv = j.template get<QString>();       break;
		case BasicJsonType::value_t::boolean:         qv = j.template get<bool>();          break;
		case BasicJsonType::value_t::number_integer:  qv = j.template get<std::int64_t>();  break;
		case BasicJsonType::value_t::number_unsigned: qv = j.template get<std::uint64_t>(); break;
		case BasicJsonType::value_t::number_float:    qv = j.template get<double>();        break;

		case BasicJsonType::value_t::binary:
		case BasicJsonType::value_t::discarded:
		default:
			qv = {};
			break;
		}
	}
	else
	{
		// TODO: impl
	}
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QList<T>>::to_json(BasicJsonType& j, const QList<T>& arr)
{
	j = BasicJsonType::array();
	for(auto&& v : arr) j.push_back(BasicJsonType(v));
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QList<T>>::from_json(const BasicJsonType& j, QList<T>& arr)
{
	for(auto&& v : j) arr.push_back(v.template get<T>());
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QMap<QString, T>>::to_json(BasicJsonType& j, const QMap<QString, T>& map)
{
	j = BasicJsonType::object();
	auto b = map.begin();
	auto e = map.end();
	for(auto i = b; i != e; ++i)
		j[i.key().toStdString()] = i.value();
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QMap<QString, T>>::from_json(const BasicJsonType& j, QMap<QString, T>& map)
{
	for(auto&& [key, value] : j.items())
		map.insert(QString::fromStdString(key), T(value));
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QHash<QString, T>>::to_json(BasicJsonType& j, const QHash<QString, T>& map)
{
	j = BasicJsonType::object();
	auto b = map.begin();
	auto e = map.end();
	for(auto i = b; i != e; ++i)
		j[i.key().toStdString()] = i.value();
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QHash<QString, T>>::from_json(const BasicJsonType& j, QHash<QString, T>& arr)
{
	for(auto&& [key, value] : j.items())
		arr.insert(QString::fromStdString(key), T(value));
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QMap<std::string, T>>::to_json(BasicJsonType& j, const QMap<std::string, T>& map)
{
	j = BasicJsonType::object();
	auto b = map.begin();
	auto e = map.end();
	for(auto i = b; i != e; ++i)
		j[i.key()] = i.value();
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QMap<std::string, T>>::from_json(const BasicJsonType& j, QMap<std::string, T>& map)
{
	for(auto&& [key, value] : j.items())
		map.insert(key, T(value));
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QHash<std::string, T>>::to_json(BasicJsonType& j, const QHash<std::string, T>& map)
{
	j = BasicJsonType::object();
	auto b = map.begin();
	auto e = map.end();
	for(auto i = b; i != e; ++i)
		j[i.key()] = i.value();
}

template<typename T>
template<typename BasicJsonType>
void adl_serializer<QHash<std::string, T>>::from_json(const BasicJsonType& j, QHash<std::string, T>& arr)
{
	for(auto&& [key, value] : j.items())
		arr.insert(key, T(value));
}

NLOHMANN_JSON_NAMESPACE_END
