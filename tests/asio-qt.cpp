#include <QApplication>
#include <QTimer>
#include <asio.hpp>
#include <Utility/EasyFmt.h>
#include <Utility/CoAsioQt.h>

using namespace std::chrono;
using namespace CoAsioQt;

class Tag : public QObject 
{
public:
    using QObject::QObject;
    
    [[noreturn]] void callback()
    {
        "[{}] tag callback"_print(std::this_thread::get_id());
        throw std::runtime_error("callback exception");
    }

    std::unique_ptr<int> withResult()
    {
        "[{}] tag withResult"_print(std::this_thread::get_id());
        return std::make_unique<int>(1);
    }
};

asio::awaitable<void> test(Tag* tag)
try {
    auto i = co_await run(tag, [t = tag, x = std::unique_ptr<int>()] { return t->withResult(); });
    "[{}] result: {}"_print(std::this_thread::get_id(), *i);

    co_await run(tag, [t = tag, x = std::unique_ptr<int>()] { t->callback(); });
    "[{}] result1"_print(std::this_thread::get_id());
}
catch(const std::exception& e)
{
    "[{}] exception: {}"_err(std::this_thread::get_id(), e.what());
    throw;
}

int main(int argc, char *argv[])
{
    "[{}] app start"_print(std::this_thread::get_id());
    QApplication app(argc, argv);
    Tag tag(&app);
    asio::io_context io;

    std::vector<std::jthread> threads;
    for(int i = 0; i < 5; ++i)
        threads.emplace_back([&io, &tag]() {
            "[{}] thread start"_print(std::this_thread::get_id());
            try {
                asio::co_spawn(io, test(&tag), [](std::exception_ptr ex) {
                    std::rethrow_exception(ex);
                });
                io.run();
            }
            catch(const std::exception& e)
            {
                "[{}] thread exception: {}"_err(std::this_thread::get_id(), e.what());
            }
            
            "[{}] io context stopped: {}"_print(std::this_thread::get_id(), io.stopped());
        });
    return app.exec();
}
