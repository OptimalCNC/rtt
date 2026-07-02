/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "unit.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string sourceRoot()
    {
        const std::string self(__FILE__);
        const std::string suffix = "/tests/rtlog_policy_test.cpp";
        std::string::size_type pos = self.rfind(suffix);
        BOOST_REQUIRE_MESSAGE(pos != std::string::npos,
                              "Could not locate RTT source root from " << self);
        return self.substr(0, pos);
    }

    std::string readFile(const std::string& path)
    {
        std::ifstream stream(path.c_str());
        BOOST_REQUIRE_MESSAGE(stream, "Could not open " << path);
        std::ostringstream buffer;
        buffer << stream.rdbuf();
        return buffer.str();
    }

    std::string stripLineComment(const std::string& line)
    {
        std::string::size_type comment = line.find("//");
        if (comment == std::string::npos)
            return line;
        return line.substr(0, comment);
    }

    bool containsAny(const std::string& text, const char* const* needles, std::size_t needle_count)
    {
        for (std::size_t i = 0; i != needle_count; ++i) {
            if (text.find(needles[i]) != std::string::npos)
                return true;
        }
        return false;
    }

    bool hasLegacyStreamLog(const std::string& line)
    {
        const std::string code = stripLineComment(line);
        const char* stream_log_starts[] = {
            "log() <<",
            "log(Info) <<",
            "log(Debug) <<",
            "log(Warning) <<",
            "log(Error) <<",
            "log(Critical) <<",
            "log(Fatal) <<",
            "log(Logger::Info) <<",
            "log(Logger::Debug) <<",
            "log(Logger::Warning) <<",
            "log(Logger::Error) <<",
            "log(Logger::Critical) <<",
            "log(Logger::Fatal) <<",
            "Logger::log() <<",
            "Logger::log(Logger::Info) <<",
            "Logger::log(Logger::Debug) <<",
            "Logger::log(Logger::Warning) <<",
            "Logger::log(Logger::Error) <<",
            "Logger::log(Logger::Critical) <<",
            "Logger::log(Logger::Fatal) <<"
        };
        return containsAny(code, stream_log_starts, sizeof(stream_log_starts) / sizeof(stream_log_starts[0])) ||
               code.find("endlog") != std::string::npos ||
               code.find("Logger::In(") != std::string::npos ||
               code.find("Logger::endl") != std::string::npos ||
               code.find("Logger::nl") != std::string::npos;
    }
}

BOOST_AUTO_TEST_SUITE(RtLogPolicyTestSuite)

BOOST_AUTO_TEST_CASE(testRealtimeSensitiveFilesUseBoundedLogger)
{
    const std::string root = sourceRoot();
    const char* files[] = {
        "rtt/base/TaskCore.cpp",
        "rtt/base/CoreRunnableInterface.cpp",
        "rtt/base/DataObjectLockFree.hpp",
        "rtt/base/InputPortInterface.cpp",
        "rtt/ConfigurationInterface.cpp",
        "rtt/deployment/ComponentLoader.cpp",
        "rtt/extras/SlaveActivity.cpp",
        "rtt/internal/BindStorage.hpp",
        "rtt/internal/ConnectionManager.cpp",
        "rtt/internal/ConnFactory.cpp",
        "rtt/internal/ConnFactory.hpp",
        "rtt/internal/LocalOperationCaller.hpp",
        "rtt/internal/OperationCallerC.cpp",
        "rtt/internal/SendHandleC.cpp",
        "rtt/internal/SharedConnection.cpp",
        "rtt/ExecutionEngine.cpp",
        "rtt/extras/FileDescriptorActivity.cpp",
        "rtt/extras/IRQActivity.cpp",
        "rtt/extras/MultiVectorComposition.hpp",
        "rtt/extras/SimulationThread.cpp",
        "rtt/DataFlowInterface.cpp",
        "rtt/marsh/CPFDemarshaller.cpp",
        "rtt/marsh/CPFMarshaller.cpp",
        "rtt/marsh/MarshallingService.cpp",
        "rtt/marsh/PropertyDemarshaller.cpp",
        "rtt/marsh/PropertyLoader.cpp",
        "rtt/marsh/PropertyMarshaller.cpp",
        "rtt/marsh/TinyDemarshaller.cpp",
        "rtt/Activity.cpp",
        "rtt/os/exceptions.cpp",
        "rtt/os/ecos/fosi_internal.cpp",
        "rtt/os/gnulinux/fosi_internal.cpp",
        "rtt/os/lxrt/fosi_internal.cpp",
        "rtt/os/macosx/fosi_internal.cpp",
        "rtt/os/main.h",
        "rtt/os/startstop.cpp",
        "rtt/os/Thread.cpp",
        "rtt/os/Timer.cpp",
        "rtt/os/win32/fosi_internal.cpp",
        "rtt/os/xenomai/fosi_internal.cpp",
        "rtt/OperationCaller.hpp",
        "rtt/plugin/PluginLoader.cpp",
        "rtt/PropertyBag.cpp",
        "rtt/Property.hpp",
        "rtt/SendHandle.hpp",
        "rtt/Service.cpp",
        "rtt/scripting/ParsedStateMachine.cpp",
        "rtt/scripting/CallFunction.hpp",
        "rtt/scripting/FunctionFactory.cpp",
        "rtt/scripting/Parser.cpp",
        "rtt/scripting/PeerParser.cpp",
        "rtt/scripting/ProgramGraphParser.cpp",
        "rtt/scripting/ScriptParser.cpp",
        "rtt/scripting/ScriptingService.cpp",
        "rtt/scripting/StatementProcessor.cpp",
        "rtt/scripting/StateMachine.cpp",
        "rtt/scripting/ValueChangeParser.cpp",
        "rtt/ServiceRequester.cpp",
        "rtt/TaskContext.cpp",
        "rtt/transports/mqueue/Dispatcher.hpp",
        "rtt/transports/mqueue/MQChannelElement.hpp",
        "rtt/transports/mqueue/MQSendRecv.cpp",
        "rtt/transports/mqueue/MQSerializationProtocol.hpp",
        "rtt/transports/mqueue/MQTemplateProtocolBase.hpp",
        "rtt/transports/corba/ApplicationServer.cpp",
        "rtt/transports/corba/ConfigurationInterfaceI.cpp",
        "rtt/transports/corba/CorbaConversion.hpp",
        "rtt/transports/corba/CorbaDispatcher.hpp",
        "rtt/transports/corba/CorbaLib.cpp",
        "rtt/transports/corba/CorbaOperationCallerFactory.cpp",
        "rtt/transports/corba/DataFlowI.cpp",
        "rtt/transports/corba/DataSourceProxy.hpp",
        "rtt/transports/corba/OperationInterfaceI.cpp",
        "rtt/transports/corba/OperationCallerProxy.hpp",
        "rtt/transports/corba/RemoteChannelElement.hpp",
        "rtt/transports/corba/RemotePorts.cpp",
        "rtt/transports/corba/ServiceI.cpp",
        "rtt/transports/corba/ServiceRequesterI.cpp",
        "rtt/transports/corba/TaskContextI.cpp",
        "rtt/transports/corba/TaskContextProxy.cpp",
        "rtt/transports/corba/TaskContextServer.cpp",
        "rtt/types/BoostArrayTypeInfo.hpp",
        "rtt/types/CArrayTypeInfo.hpp",
        "rtt/types/EnumTypeInfo.hpp",
        "rtt/types/SequenceTypeInfoBase.hpp",
        "rtt/types/StructTypeInfo.hpp",
        "rtt/types/TemplateCompositionFactory.hpp",
        "rtt/types/TemplateConstructor.hpp",
        "rtt/types/PropertyComposition.cpp",
        "rtt/types/PropertyDecomposition.cpp",
        "rtt/types/TypeInfo.cpp",
        "rtt/types/TypeInfoRepository.cpp",
        "rtt/types/TypekitRepository.cpp",
        "rtt/types/VectorTemplateComposition.hpp",
        "tests/test-main.cpp",
        "tests/test-runner.cpp",
        "tests/test-runner-corba.cpp"
    };

    std::vector<std::string> violations;
    for (std::size_t file_index = 0; file_index != sizeof(files) / sizeof(files[0]); ++file_index) {
        const std::string relative = files[file_index];
        std::istringstream lines(readFile(root + "/" + relative));
        std::string line;
        int line_number = 0;
        while (std::getline(lines, line)) {
            ++line_number;
            if (hasLegacyStreamLog(line)) {
                std::ostringstream message;
                message << relative << ":" << line_number << ": " << line;
                violations.push_back(message.str());
            }
        }
    }

    BOOST_CHECK_MESSAGE(violations.empty(),
                        "RT-sensitive logging must use Logger::log().logf(); first violation: "
                        << (violations.empty() ? "" : violations.front()));
}

BOOST_AUTO_TEST_SUITE_END()
