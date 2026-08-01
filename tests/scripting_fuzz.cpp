#include <scripting/ConditionInterface.hpp>
#include <scripting/Parser.hpp>
#include <scripting/ProgramExceptions.hpp>
#include <scripting/ScriptingService.hpp>

#include <Logger.hpp>
#include <Service.hpp>
#include <TaskContext.hpp>
#include <base/DataSourceBase.hpp>
#include <os/startstop.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

#ifndef RTT_FUZZ_COMPONENT_PATH
#define RTT_FUZZ_COMPONENT_PATH "../rtt"
#endif

namespace {

class OrocosFuzzRuntime
{
public:
    OrocosFuzzRuntime()
    {
#if defined(_WIN32)
        _putenv_s("RTT_COMPONENT_PATH", RTT_FUZZ_COMPONENT_PATH);
#else
        setenv("RTT_COMPONENT_PATH", RTT_FUZZ_COMPONENT_PATH, 1);
#endif
        if (__os_init(1, arguments_) != 0)
            std::abort();
        RTT::Logger::log().setLogLevel(RTT::Logger::Critical);
    }

    ~OrocosFuzzRuntime()
    {
        __os_exit();
    }

private:
    char program_name_[32] = "rtt_scripting_fuzzer";
    char* arguments_[2] = {program_name_, nullptr};
};

class ParserFuzzFixture
{
public:
    ParserFuzzFixture()
        : task_("scripting_fuzz"), counter_(0),
          test_service_(RTT::Service::Create("test", &task_)),
          scripting_service_(RTT::scripting::ScriptingService::Create(&task_))
    {
        test_service_->addOperation(
            "increase", &ParserFuzzFixture::increase, this);
        test_service_->addOperation(
            "getState", &ParserFuzzFixture::getState, this);
    }

    RTT::TaskContext& task()
    {
        return task_;
    }

    RTT::scripting::ScriptingService* scriptingService()
    {
        return scripting_service_.get();
    }

private:
    int increase()
    {
        return ++counter_;
    }

    std::vector<double> getState(int state) const
    {
        return {
            static_cast<double>(state),
            static_cast<double>(state + 1)
        };
    }

    RTT::TaskContext task_;
    int counter_;
    RTT::Service::shared_ptr test_service_;
    RTT::scripting::ScriptingService::shared_ptr scripting_service_;
};

void exerciseParser(const std::string& input, std::uint8_t mode)
{
    ParserFuzzFixture fixture;
    RTT::TaskContext& task = fixture.task();
    RTT::scripting::Parser parser(task.engine());

    switch (mode % 6U) {
    case 0: {
        RTT::base::DataSourceBase::shared_ptr result =
            parser.parseExpression(input, &task);
        if (result)
            result->evaluate();
        break;
    }
    case 1: {
        std::unique_ptr<RTT::scripting::ConditionInterface> condition(
            parser.parseCondition(input, &task));
        if (condition)
            condition->evaluate();
        break;
    }
    case 2: {
        RTT::base::DataSourceBase::shared_ptr result =
            parser.parseValueStatement(input, &task);
        if (result)
            result->evaluate();
        break;
    }
    case 3:
        parser.parseProgram(input, &task, "fuzz-program");
        break;
    case 4:
        parser.parseStateMachine(input, &task, "fuzz-state-machine");
        break;
    case 5:
        parser.runScript(
            input, &task, fixture.scriptingService(), "fuzz-script");
        break;
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(
    const std::uint8_t* data, std::size_t size)
{
    static OrocosFuzzRuntime runtime;
    (void)runtime;

    if (size == 0)
        return 0;

    const std::string input(
        reinterpret_cast<const char*>(data + 1), size - 1U);
    try {
        exerciseParser(input, data[0]);
    } catch (const RTT::file_parse_exception&) {
    } catch (const RTT::parse_exception&) {
    } catch (const RTT::scripting::program_load_exception&) {
    }
    return 0;
}
