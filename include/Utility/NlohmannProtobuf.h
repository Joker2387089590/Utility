#pragma once
#include <google/protobuf/struct.pb.h>
#include <nlohmann/json.hpp>

// Protobuf Struct 与 nlohmann-json 类型互相转换
// 参考 https://json.nlohmann.me/features/arbitrary_types/#how-do-i-convert-third-party-types

NLOHMANN_JSON_NAMESPACE_BEGIN

/// declare

template<>
struct adl_serializer<google::protobuf::Struct>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const google::protobuf::Struct& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, google::protobuf::Struct& s);
};

template<>
struct adl_serializer<google::protobuf::ListValue>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const google::protobuf::ListValue& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, google::protobuf::ListValue& s);
};

template<>
struct adl_serializer<google::protobuf::Value>
{
	template<typename BasicJsonType>
	static void to_json(BasicJsonType& j, const google::protobuf::Value& s);

	template<typename BasicJsonType>
	static void from_json(const BasicJsonType& j, google::protobuf::Value& s);
};

/// impl

template<typename BasicJsonType>
void adl_serializer<google::protobuf::Struct>::to_json(BasicJsonType& j, const google::protobuf::Struct& s)
{
	j = BasicJsonType::object();
	for(auto&& [key, value] : s.fields()) j.emplace(key, value);
}

template<typename BasicJsonType>
void adl_serializer<google::protobuf::Struct>::from_json(const BasicJsonType& j, google::protobuf::Struct& s)
{
	auto* f = s.mutable_fields();
	for(auto&& [key, value] : j.items())
		f->emplace(key, value.template get<google::protobuf::Value>());
}

template<typename BasicJsonType>
void adl_serializer<google::protobuf::ListValue>::to_json(BasicJsonType& j, const google::protobuf::ListValue& arr)
{
	j = BasicJsonType::array();
	for(auto&& v : arr.values()) j.push_back(v);
}

template<typename BasicJsonType>
void adl_serializer<google::protobuf::ListValue>::from_json(const BasicJsonType& j, google::protobuf::ListValue& arr)
{
	for(auto&& v : j) v.get_to(*arr.add_values());
}

template<typename BasicJsonType>
void adl_serializer<google::protobuf::Value>::to_json(BasicJsonType& j, const google::protobuf::Value& value)
{
	switch(value.kind_case())
	{
	case google::protobuf::Value::kBoolValue: j = value.bool_value(); break;
	case google::protobuf::Value::kNumberValue: j = value.number_value(); break;
	case google::protobuf::Value::kStringValue: j = value.string_value(); break;
	case google::protobuf::Value::kStructValue: j = value.struct_value(); break;
	case google::protobuf::Value::kListValue: j = value.list_value(); break;
	case google::protobuf::Value::kNullValue: j = {}; break;
	default: break;
	}
}

template<typename BasicJsonType>
void adl_serializer<google::protobuf::Value>::from_json(const BasicJsonType& j, google::protobuf::Value& value)
{
	switch(j.type())
	{
	case BasicJsonType::value_t::object: j.get_to(*value.mutable_struct_value()); break;
	case BasicJsonType::value_t::array: j.get_to(*value.mutable_list_value()); break;
	case BasicJsonType::value_t::string: j.get_to(*value.mutable_string_value()); break;
	case BasicJsonType::value_t::boolean: value.set_bool_value(j.template get<bool>()); break;
	case BasicJsonType::value_t::null: value.set_null_value({}); break;
	case BasicJsonType::value_t::number_integer:
	case BasicJsonType::value_t::number_unsigned:
	case BasicJsonType::value_t::number_float:
		value.set_number_value(j.template get<double>()); break;
	default: break;
	}
}

NLOHMANN_JSON_NAMESPACE_END
