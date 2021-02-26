/*
 * Copyright (C) 2017-2021 Canonical, Ltd.
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

#include "client.h"
#include "cmd/alias.h"
#include "cmd/aliases.h"
#include "cmd/delete.h"
#include "cmd/exec.h"
#include "cmd/find.h"
#include "cmd/get.h"
#include "cmd/help.h"
#include "cmd/info.h"
#include "cmd/launch.h"
#include "cmd/list.h"
#include "cmd/mount.h"
#include "cmd/networks.h"
#include "cmd/purge.h"
#include "cmd/recover.h"
#include "cmd/restart.h"
#include "cmd/set.h"
#include "cmd/shell.h"
#include "cmd/start.h"
#include "cmd/stop.h"
#include "cmd/suspend.h"
#include "cmd/transfer.h"
#include "cmd/umount.h"
#include "cmd/unalias.h"
#include "cmd/version.h"

#include <algorithm>

#include <multipass/cli/argparser.h>
#include <multipass/cli/client_common.h>
#include <multipass/logging/log.h>
#include <multipass/platform.h>

namespace mp = multipass;
namespace mpl = multipass::logging;

mp::Client::Client(ClientConfig& config)
    : cert_provider{std::move(config.cert_provider)},
      rpc_channel{mp::client::make_channel(config.server_address, config.conn_type, *cert_provider)},
      stub{mp::Rpc::NewStub(rpc_channel)},
      term{config.term}
{
    add_command<cmd::Alias>();
    add_command<cmd::Aliases>();
    add_command<cmd::Launch>();
    add_command<cmd::Purge>();
    add_command<cmd::Exec>();
    add_command<cmd::Find>();
    add_command<cmd::Get>();
    add_command<cmd::Help>();
    add_command<cmd::Info>();
    add_command<cmd::List>();
    add_command<cmd::Networks>();
    add_command<cmd::Mount>();
    add_command<cmd::Recover>();
    add_command<cmd::Set>();
    add_command<cmd::Shell>();
    add_command<cmd::Start>();
    add_command<cmd::Stop>();
    add_command<cmd::Suspend>();
    add_command<cmd::Transfer>();
    add_command<cmd::Unalias>();
    add_command<cmd::Restart>();
    add_command<cmd::Delete>();
    add_command<cmd::Umount>();
    add_command<cmd::Version>();

    sort_commands();
}

void mp::Client::sort_commands()
{
    auto name_sort = [](cmd::Command::UPtr& a, cmd::Command::UPtr& b) { return a->name() < b->name(); };
    std::sort(commands.begin(), commands.end(), name_sort);
}

int mp::Client::run(const QStringList& arguments)
{
    QString description("Create, control and connect to Ubuntu instances.\n\n"
                        "This is a command line utility for multipass, a\n"
                        "service that manages Ubuntu instances.");

    ArgParser parser(arguments, commands, term->cout(), term->cerr());
    parser.setApplicationDescription(description);

    ParseCode parse_status = parser.parse();

    if (!mpl::get_logger())
        mp::client::set_logger(mpl::level_from(parser.verbosityLevel())); // we need logging for...
    mp::client::pre_setup(); // ... something we want to do even if the command was wrong

    const auto ret =
        parse_status == ParseCode::Ok ? parser.chosenCommand()->run(&parser) : parser.returnCodeFrom(parse_status);

    mp::client::post_setup();

    return ret;
}
