#pragma once
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QDateTime>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonDocument>
#include <range/v3/view/zip.hpp>
#include <Utility/EasyFmt.h>
#include <Utility/TypeList.h>

/// 基于 Qt::Sql sqlite3 的工具类与工具函数
namespace Sqlite3
{
using namespace std::literals;
using namespace Qt::Literals;

template<typename T>
QString lastError(const T& query) { return query.lastError().text(); }

// 以下没有标记 noexcept 的函数，有可能主动抛出 SqlException
class SqlException : public std::runtime_error
{
public:
	explicit SqlException(const QSqlQuery& query, const QString& detail) :
		std::runtime_error("{}: {}"_fmt(detail, lastError(query)))
	{}

	explicit SqlException(const QSqlDatabase& db, const QString& detail) :
		std::runtime_error("{}: {}"_fmt(detail, lastError(db)))
	{}

	SqlException(const SqlException&) = default;
	SqlException(SqlException&&) noexcept = default;
};

inline void prepare(QSqlQuery& query, const QString& statement)
{
	using namespace Qt::StringLiterals;
	if(!query.prepare(statement))
		throw SqlException(query, u"prepare '%1' failed"_s.arg(statement));
}

inline QString execWithError(QSqlQuery& query)
{
	return query.exec() ? QString{} : lastError(query);
}

inline QString execWithError(QSqlQuery& query, const QString& statement)
{
	return query.exec(statement) ? QString{} : lastError(query);
}

inline void execOrThrow(QSqlQuery& query, const QString& statement)
{
	if(!query.exec(statement)) throw SqlException(query, statement);
}

inline void execOrThrow(const QSqlDatabase& db, const QString& statement)
{
	QSqlQuery query(db);
	return execOrThrow(query, statement);
}

inline void execOrThrow(QSqlQuery& query)
{
	if(!query.exec()) throw SqlException(query, query.lastQuery());
}

struct FieldPack;

template<typename T, auto& text>
struct Field
{
	using type = T;
	inline static constexpr std::string_view typeText = text;
	inline static constexpr auto meta = QMetaType::fromType<T>();
	FieldPack operator()(const QString& name, QStringList constraints) const noexcept;

	template<typename... Cs, std::enable_if_t<(std::is_constructible_v<QString, Cs> && ...), int> = 0>
	FieldPack operator()(const QString& name, Cs&&... constraints) const noexcept;
};

/// 封装 sqlite3 的类型系统，见 https://www.sqlite.org/datatype3.html
/// NOTE:
/// * sqlite3 有 "存储类型" 和 "亲和类型" 的概念，以下是根据 "亲和类型" 封装的，
///   但对 BOOLEAN 进行了单独处理
///	* sqlite3 存入的一个数据的类型，可以创建表时声明的字段类型不一致
///	  参见 https://www.sqlite.org/flextypegood.html
namespace Fields
{
inline constexpr char typeTextTEXT[] = "TEXT";
inline constexpr char typeTextNUMERIC[] = "NUMERIC";
inline constexpr char typeTextINTEGER[] = "INTEGER";
inline constexpr char typeTextREAL[] = "REAL";
inline constexpr char typeTextBLOB[] = "BLOB";
inline constexpr char typeTextBOOLEAN[] = "BOOLEAN";

inline constexpr Field<QString, typeTextTEXT> text;
inline constexpr Field<double, typeTextNUMERIC> numeric;
inline constexpr Field<int, typeTextINTEGER> integer;
inline constexpr Field<double, typeTextREAL> real;
inline constexpr Field<QByteArray, typeTextBLOB> blob;
inline constexpr Field<bool, typeTextBOOLEAN> boolean;
}

// Variants::Variant<decltype(integer), ...>
using VarField =
	Types::TList<
		std::remove_const_t<decltype(Fields::text)>,
		std::remove_const_t<decltype(Fields::numeric)>,
		std::remove_const_t<decltype(Fields::integer)>,
		std::remove_const_t<decltype(Fields::real)>,
		std::remove_const_t<decltype(Fields::blob)>,
		std::remove_const_t<decltype(Fields::boolean)>
	>::Apply<std::variant>;

struct FieldPack
{
	FieldPack(VarField type, QString name, QStringList constraints) noexcept :
		type(type),
		name(std::move(name)),
		constraints(std::move(constraints))
	{}

	FieldPack(const FieldPack&) = default;
	FieldPack& operator=(const FieldPack&) & = default;
	FieldPack(FieldPack&&) noexcept = default;
	FieldPack& operator=(FieldPack&&) & noexcept = default;
	~FieldPack() = default;

	VarField type;
	QString name;
	QStringList constraints;
};

template<typename T, auto& x>
FieldPack Field<T, x>::operator()(const QString& name, QStringList constraints) const noexcept
{
	return {*this, name, std::move(constraints)};
}

template<typename T, auto& text>
template<typename... Cs, std::enable_if_t<(std::is_constructible_v<QString, Cs> && ...), int>>
FieldPack Field<T, text>::operator()(const QString& name, Cs&&... constraints) const noexcept
{
	return (*this)(name, QStringList{QString(constraints)...});
}

class FieldList
{
public:
	FieldList(std::initializer_list<FieldPack> fs) : fields(fs) {}
	explicit FieldList(std::vector<FieldPack> fs) noexcept : fields(std::move(fs)) {}

	auto& allFields() const noexcept { return fields; }
	std::size_t size() const noexcept { return fields.size(); }
	bool isEmpty() const noexcept { return fields.empty(); }
	bool empty() const noexcept { return fields.empty(); }

	[[nodiscard]] QStringList headers() const noexcept
	{
		QStringList headers;
		for(auto&& f : fields) headers.append(f.name);
		return headers;
	}

	// ``` "field 1", "field 2", "field 3", ... ```
	[[nodiscard]] QString mergedHeader() const noexcept
	{
		QStringList headers;
		for(auto&& f : fields) headers.append('"' + f.name + '"');
		return headers.join(", ");
	}

	[[nodiscard]] int posOf(const QString& field) const noexcept
	{
		auto b = fields.rbegin();
		auto e = fields.rend();
		auto pos = std::find_if(b, e, [&](const FieldPack& f) { return f.name == field; });
		return int(e - pos) - 1;
	}

	[[nodiscard]] QSqlRecord record(const QString& table = {}) const
	{
		QSqlRecord record;
		for(auto& field : fields)
		{
			auto meta = std::visit([](auto& type) { return type.meta; }, field.type);
			record.append(QSqlField(field.name, meta, table));
		}
		return record;
	}

	[[nodiscard]] QSqlRecord record(const QVariantList& values) const
	{
		assert(values.size() == int(fields.size()));
		QSqlRecord record;
		for(auto&& [field, value] : ranges::views::zip(fields, values))
		{
			auto meta = std::visit([](auto& type) { return type.meta; }, field.type);
			auto qField = QSqlField(field.name, meta);
			qField.setValue(value);
			record.append(qField);
		}
		return record;
	}

	// https://sqlite.org/lang_createtable.html
	struct CreateOptions
	{
		// https://github.com/llvm/llvm-project/issues/36032#issuecomment-1284315717
		CreateOptions() {}

		QStringList constraints{};
		bool temporary{false};
		bool ifNotExists{true};
		bool withoutRowId{false};
		bool strict{false};
	};

	[[nodiscard]] QString create(const QString& tableName, CreateOptions op = {}) const noexcept
	{
		assert(!tableName.isEmpty());

		QStringList columns;
		for(auto&& f : fields)
		{
			auto typeName = std::visit([](auto& t) { return t.typeText; }, f.type);
			columns.append("'{}' {} {}"_qfmt(f.name, typeName, f.constraints.join(u' ')));
		}

		static constexpr std::array options{
			u""sv,
			u"WITHOUT ROWID"sv,
			u"STRICT"sv,
			u"WITHOUT ROWID, STRICT"sv,
		};
		auto option = options[(op.withoutRowId ? 0b01 : 0b00) + (op.strict ? 0b10 : 0b00)];

		auto statement = fmt::format(u"CREATE {} TABLE {} [{}] ({}) {}",
			op.temporary ? u"TEMPORARY"sv : u""sv,
			op.ifNotExists ? u"IF NOT EXISTS"sv : u""sv,
			tableName,
			(columns + op.constraints).join(", "),
			option
		);
		return QString::fromStdU16String(statement);
	}

	[[nodiscard]] QString insertWithPlaceholder(const QString& tableName) const noexcept
	{
		auto values = u"?, "_s.repeated(qsizetype(fields.size())).chopped(2);
		return u"INSERT INTO [{}] ({}) VALUES ({})"_qfmt(tableName, mergedHeader(), values);
	}

	[[nodiscard]] QSqlQuery preparedQuery(const QSqlDatabase& db, const QString& table) const
	{
		QSqlQuery query(db);
		prepare(query, insertWithPlaceholder(table));
		return query;
	}

	void insertPrepared(QSqlQuery& query, const QVariantList& datas) const
	{
		assert(int(fields.size()) == datas.size());
		for(auto& data : datas) query.addBindValue(data);
		execOrThrow(query);
	}

	template<typename C>
	void insert(const QSqlDatabase& db, const QString& table, const C& datas) const
	{
		auto query = preparedQuery(db, table);
		for(const auto& row : datas)
		{
			static_assert(std::is_convertible_v<decltype(row), QVariantList>);
			insertPrepared(query, row);
		}
	}

	template<template<typename...> typename Container = QList>
	Container<QVariantList> selectAll(const QSqlDatabase& db, const QString& table) const
	{
		auto query = QSqlQuery(db);
		execOrThrow(query, u"SELECT {} FROM [{}]"_qfmt(mergedHeader(), table));

		Container<QVariantList> records;
		while(query.next())
		{
			auto record = query.record();
			QVariantList row;
			row.reserve(qsizetype(size()));
			for(auto& field : fields)
				row.push_back(record.value(field.name));
			records.emplace_back(std::move(row));
		}
		return records;
	}

	friend FieldList operator+(const FieldList& a, const FieldList& b) noexcept
	{
		auto& afields = a.allFields();
		auto& bfields = b.allFields();
		std::vector<FieldPack> fs;
		fs.reserve(afields.size() + bfields.size());
		auto i = std::copy(afields.begin(), afields.end(), std::back_inserter(fs));
		std::copy(bfields.begin(), bfields.end(), i);
		return FieldList(std::move(fs));
	}

private:
	std::vector<FieldPack> fields;
};

template<typename Tx>
QVariant asVariant(Tx&& value)
{
	using T = std::decay_t<Tx>;
	if constexpr (std::is_same_v<T, std::string>)
		return QString::fromStdString(value);
	else if constexpr (std::is_same_v<T, QDateTime>)
	{
		// 可用 ISO8601 格式的字符串在 SQLite3 中存储时间
		// 对应 QDateTime().toString(Qt::ISODateWithMs)
		// https://www.sqlite.org/datatype3.html#date_and_time_datatype
		return value.toString(Qt::ISODateWithMs);
	}
	else if constexpr (std::is_same_v<T, QJsonValue>)
	{
		// JSON 转成文本，但 QJsonValue 不支持直接转文本，所以套一层 QJsonObject
		auto tmp = QJsonObject{ {"v", std::forward<Tx>(value)} };
		return QJsonDocument(tmp).toJson();
	}
	else
	{
		return QVariant::fromValue(std::forward<Tx>(value));
	}
}

template<typename T>
inline constexpr auto fromVariant = [](const QVariant& v) { return v.value<T>(); };

template<>
inline constexpr auto fromVariant<std::string> = [](const QVariant& v) {
	return v.toString().toStdString();
};

template<>
inline constexpr auto fromVariant<QJsonValue> = [](const QVariant& v) {
	auto obj = QJsonDocument::fromJson(v.toByteArray());
	return obj["v"];
};

template<>
inline constexpr auto fromVariant<QDateTime> = [](const QVariant& v) {
	return QDateTime::fromString(v.toString(), Qt::ISODateWithMs);
};

template<typename T>
void asValue(const QVariant& v, T& t) { t = fromVariant<T>(v); }

// 从数据库构造一个 FieldList
// https://www.sqlite.org/pragma.html#pragma_table_info
inline FieldList headers(const QSqlDatabase& db, const QString& tableName)
{
	auto query = QSqlQuery(db);
	execOrThrow(
		query, uR"(SELECT name, type, notnull FROM pragma_table_info([{}]))"_qfmt(tableName)
	);

	std::vector<FieldPack> header;
	while(query.next())
	{
		auto name = query.value(0).toString();
		auto type = query.value(1).toString();
		auto notNull = query.value(2).toBool();

		// https://www.sqlite.org/datatype3.html#determination_of_column_affinity
		auto vtype = [&type]() -> VarField {
			if(type.contains(u"BOOL"))
				return Fields::boolean;
			else if(type.contains(u"INT"))
				return Fields::integer;
			else if(type.contains(u"CHAR") || type.contains(u"CLOB") || type.contains(u"TEXT"))
				return Fields::text;
			else if(type.contains(u"BLOB") || type.isEmpty())
				return Fields::blob;
			else if(type.contains(u"REAL") || type.contains(u"FLOA") || type.contains(u"DOUB"))
				return Fields::real;
			else
				return Fields::numeric;
		} ();

		QStringList constraints;
		if(notNull) constraints.append("NOT NULL");

		header.emplace_back(std::move(vtype), name, std::move(constraints));
	}

	return FieldList(header);
}

// 从表中生成 CSV，返回的是 UTF-8 编码的文本，可以直接用于写入文件，不用转 QString
inline QByteArray csv(const QSqlDatabase& db, const QString& tableName, QStringList fields = {})
{
	QSqlQuery query(db);
	if(fields.isEmpty())
	{
		// 导出全部列
		execOrThrow(query, uR"(SELECT name FROM pragma_table_info([{}]))"_qfmt(tableName));
		while(query.next()) fields.append(query.value(0).toString());
	}

	// 用 || 把字段名连起来，中间插入逗号
	auto mergeFields = fields.join(" || ', ' || ");
	auto selectRow = uR"(SELECT {} as one_row FROM [{}];)"_qfmt(mergeFields, tableName);

	// 用 group_concat 合并行，char(10) == \n
	auto statement = uR"(SELECT group_concat(one_row, char(10)) FROM ({}) sub;)"_qfmt(selectRow);

	execOrThrow(query, statement);

	[[maybe_unused]] bool hasResult = query.next();
	assert(hasResult);

	QByteArray result;
	result += fields.join(", ").toUtf8();
	result += '\n';
	result += query.value(0).toByteArray();

	assert(!query.next()); // query 的结果只有一行一列，一次就可以全部读出来了
	return result;
}
} // namespace Sqlite3
