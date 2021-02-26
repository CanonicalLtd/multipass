/*
 * Copyright (C) 2021 Canonical, Ltd.
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

#include "unalias.h"
#include "common_cli.h"

#include <multipass/cli/argparser.h>

namespace mp = multipass;
namespace cmd = multipass::cmd;

mp::ReturnCode cmd::Unalias::run(mp::ArgParser* parser)
{
    // TODO
    return ReturnCode::Ok;
}

std::string cmd::Unalias::name() const
{
    return "unalias";
}

QString cmd::Unalias::short_help() const
{
    return QStringLiteral("Remove an alias");
}

QString cmd::Unalias::description() const
{
    return QStringLiteral("Remove an alias");
}

mp::ParseCode cmd::Unalias::parse_args(mp::ArgParser* parser)
{
    parser->addPositionalArgument("name", "The name of the alias to remove\n");

    return ParseCode::Ok;
}
