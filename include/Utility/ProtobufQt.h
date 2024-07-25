#pragma once
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <google/protobuf/struct.pb.h>
#include <google/protobuf/any.pb.h>
#include <google/protobuf/util/json_util.h>
#include <Utility/TypeList.h>

// Qt JSON 与 Protobuf 类型互相转换
// 参考
// https://protobuf.dev/programming-guides/proto3/#json
// https://protobuf.dev/reference/protobuf/google.protobuf/#struct
// https://protobuf.dev/reference/protobuf/google.protobuf/#list-value
// https://protobuf.dev/reference/protobuf/google.protobuf/#value
// https://protobuf.dev/reference/protobuf/google.protobuf/#null-value

namespace ProtobufQt
{
struct JsonConverter
{
	static QJsonValue toQt(const google::protobuf::Value& v){
    using google::protobuf::Value;
    switch(v.kind_case())
    {
    case Value::kNullValue:   return QJsonValue::Type::Null;
    case Value::kNumberValue: return v.number_value();
    case Value::kStringValue: return QString::fromStdString(v.string_value());
    case Value::kBoolValue:   return v.bool_value();
    case Value::kStructValue: return toQt(v.struct_value());
    case Value::kListValue:   return toQt(v.list_value());
	case Value::KIND_NOT_SET:
	default:
		return QJsonValue::Type::Undefined;
    }
}
	static QJsonObject toQt(const google::protobuf::Struct& obj){
    QJsonObject qjson;
    for(auto& [k, v] : obj.fields())
        qjson.insert(QString::fromStdString(k), toQt(v));
    return qjson;
}
	static QJsonArray toQt(const google::protobuf::ListValue& arr){
    QJsonArray qjson;
    for(auto& v : arr.values()) qjson.append(toQt(v));
	return qjson;
}

	static void toProtobuf(const QJsonObject& obj, google::protobuf::Struct* s){
	const auto b = obj.begin();
	const auto e = obj.end();
	auto fields = s->mutable_fields();
	for(auto i = b; i != e; ++i)
	{
		auto [pos, success] = fields->try_emplace(i.key().toStdString());
		if(!success)
			[[unlikely]] continue;
		else
			toProtobuf(i.value(), &pos->second);
	}
}
	static void toProtobuf(const QJsonArray& arr, google::protobuf::ListValue* list){
	list->clear_values();
	const auto b = arr.begin();
	const auto e = arr.end();
	for(auto i = b; i != e; ++i) toProtobuf(*i, list->add_values());
}
	static void toProtobuf(const QJsonValue& qv, google::protobuf::Value* pv){
	switch(qv.type())
	{
	case QJsonValue::Null: [[unlikely]]
		pv->set_null_value({});
		break;
	case QJsonValue::Bool:
		pv->set_bool_value(qv.toBool());
		break;
	case QJsonValue::Double:
		pv->set_number_value(qv.toDouble());
		break;
	case QJsonValue::String:
		pv->set_string_value(qv.toString().toStdString());
		break;
	case QJsonValue::Array:
		toProtobuf(qv.toArray(), pv->mutable_list_value());
		break;
	case QJsonValue::Object:
		toProtobuf(qv.toObject(), pv->mutable_struct_value());
		break;
	case QJsonValue::Undefined: [[unlikely]]
		break;
	}
}
	static void toProtobuf(QJsonValueConstRef qv, google::protobuf::Value* pv){
	switch(qv.type())
	{
	case QJsonValue::Null: [[unlikely]]
		pv->set_null_value({});
		break;
	case QJsonValue::Bool:
		pv->set_bool_value(qv.toBool());
		break;
	case QJsonValue::Double:
		pv->set_number_value(qv.toDouble());
		break;
	case QJsonValue::String:
		pv->set_string_value(qv.toString().toStdString());
		break;
	case QJsonValue::Array:
		toProtobuf(qv.toArray(), pv->mutable_list_value());
		break;
	case QJsonValue::Object:
		toProtobuf(qv.toObject(), pv->mutable_struct_value());
		break;
	case QJsonValue::Undefined: [[unlikely]]
		break;
	}
}
};

class anyToQJson
{
public:
	anyToQJson(const google::protobuf::Any& msg) : msg(msg)
	{
		assert(msg.Is<google::protobuf::Struct>() ||
			   msg.Is<google::protobuf::ListValue>() ||
			   msg.Is<google::protobuf::Value>());
	}

	using TL = Types::TList<QJsonObject, QJsonArray, QJsonValue>;

	template<typename T, typename U>
	using IsValid = std::is_same<std::decay_t<T>, U>;

	template<typename T, std::enable_if_t<TL::anyIs<T, IsValid>, int> = 0>
	operator T() const&&
	{
		using U = std::decay_t<T>;
		if constexpr (std::is_same_v<U, QJsonObject>)
		{
			google::protobuf::Struct proto;
			msg.UnpackTo(&proto);
			return JsonConverter::toQt(proto);
		}
		else if constexpr (std::is_same_v<U, QJsonArray>)
		{
			google::protobuf::ListValue proto;
			msg.UnpackTo(&proto);
			return JsonConverter::toQt(proto);
		}
		else
		{
			google::protobuf::Value proto;
			msg.UnpackTo(&proto);
			return JsonConverter::toQt(proto);
		}
	}

private:
	const google::protobuf::Any& msg;
};

inline bool toProtoJson(const QJsonObject& obj, google::protobuf::Any* any)
{
	google::protobuf::Struct proto;
	JsonConverter::toProtobuf(obj, &proto);
	return any->PackFrom(proto);
}

inline bool toProtoJson(const QJsonArray& arr, google::protobuf::Any* any)
{
	google::protobuf::ListValue proto;
	JsonConverter::toProtobuf(arr, &proto);
	return any->PackFrom(proto);
}

inline bool toProtoJson(const QJsonValue& v, google::protobuf::Any* any)
{
	google::protobuf::Value proto;
	JsonConverter::toProtobuf(v, &proto);
	return any->PackFrom(proto);
}
} // namespace GtLab::Tools
