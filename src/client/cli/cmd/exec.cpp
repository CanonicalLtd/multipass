/*
 * Copyright (C) 2017-2018 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "exec.h"
#include "common_cli.h"

#include <multipass/cli/argparser.h>
#include <multipass/ssh/ssh_client.h>

namespace mp = multipass;
namespace cmd = multipass::cmd;
using RpcMethod = mp::Rpc::Stub;

mp::ReturnCode cmd::Exec::run(mp::ArgParser* parser)
{
    auto ret = parse_args(parser);
    if (ret != ParseCode::Ok)
    {
        return parser->returnCodeFrom(ret);
    }

    std::vector<std::string> args;

    auto execute_alias = parser->executeAlias();

    if (execute_alias)
    {
        args.push_back(execute_alias->command);

        for (const auto& arg : execute_alias->arguments)
            args.push_back(arg);
    }
    else
    {
        for (int i = 1; i < parser->positionalArguments().size(); ++i)
            args.push_back(parser->positionalArguments().at(i).toStdString());
    }

    auto on_success = [this, &args](mp::SSHInfoReply& reply) { return exec_success(reply, args, term); };

    auto on_failure = [this](grpc::Status& status) { return standard_failure_handler_for(name(), cerr, status); };

    request.set_verbosity_level(parser->verbosityLevel());
    return dispatch(&RpcMethod::ssh_info, request, on_success, on_failure);
}

std::string cmd::Exec::name() const
{
    return "exec";
}

QString cmd::Exec::short_help() const
{
    return QStringLiteral("Run a command on an instance");
}

QString cmd::Exec::description() const
{
    return QStringLiteral("Run a command on an instance");
}

mp::ReturnCode cmd::Exec::exec_success(const mp::SSHInfoReply& reply, const std::vector<std::string>& args,
                                       mp::Terminal* term)
{
    // TODO: mainly for testing - need a better way to test parsing
    if (reply.ssh_info().empty())
        return ReturnCode::Ok;

    const auto& ssh_info = reply.ssh_info().begin()->second;
    const auto& host = ssh_info.host();
    const auto& port = ssh_info.port();
    const auto& username = ssh_info.username();
    const auto& priv_key_blob = ssh_info.priv_key_base64();

    try
    {
        auto console_creator = [&term](auto channel) { return Console::make_console(channel, term); };
        mp::SSHClient ssh_client{host, port, username, priv_key_blob, console_creator};
        return static_cast<mp::ReturnCode>(ssh_client.exec(args));
    }
    catch (const std::exception& e)
    {
        term->cerr() << "exec failed: " << e.what() << "\n";
        return ReturnCode::CommandFail;
    }
}

mp::ParseCode cmd::Exec::parse_args(mp::ArgParser* parser)
{
    parser->addPositionalArgument("name", "Name of instance to execute the command on", "<name>");
    parser->addPositionalArgument("command", "Command to execute on the instance", "[--] <command>");

    auto status = parser->commandParse(this);

    if (status != ParseCode::Ok)
    {
        if (!parser->unknownOptionNames().empty() && !parser->containsArgument("--"))
        {
            cerr << "\nOptions to the inner command should come after \"--\", like this:\nmultipass exec <instance> -- "
                    "<command> <arguments>\n";
        }
        return status;
    }

    auto execute_alias = parser->executeAlias();
    auto arg_count = parser->positionalArguments().count();

    if ((execute_alias && arg_count > 0) || (!execute_alias && arg_count < 2))
    {
        cerr << "Wrong number of arguments\n";
        status = ParseCode::CommandLineError;
    }
    else
    {
        auto entry = request.add_instance_name();
        entry->append(execute_alias ? execute_alias->instance : parser->positionalArguments().first().toStdString());
    }

    return status;
}
