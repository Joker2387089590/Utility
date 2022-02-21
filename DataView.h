#include <stdexcept>
#include <vector>

template<typename T>
struct DataView
{
	DataView() noexcept = default;

	DataView(T* d, std::size_t s) noexcept : data(d), size(s) {}

	DataView(std::vector<std::remove_const_t<T>>& buf) noexcept :
		data(buf.data()), size(buf.size())
	{}

	T* data;
	std::size_t size;
};

struct RawView : public DataView<void>
{
	using DataView::DataView;

	template<typename T>
	RawView(DataView<T> view) noexcept :
		DataView<void>(view.data, view.size * sizeof(T)) {}

	template<typename T>
	auto cast() const
	{
		if(size % sizeof(T) != 0) throw std::logic_error("Bad DataView cast!");
		return DataView<T>{ static_cast<T*>(data), size / sizeof(T) };
	}
};
