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

#include "client_formatter.h"

#include <multipass/utils.h>

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <sstream>

namespace mp = multipass;
namespace mpu = multipass::utils;

std::string mp::ClientFormatter::format(const mp::AliasDict& aliases) const
{
    std::string formatted_output;

    if (preferred_format == "csv")
        formatted_output = format_csv(aliases);
    else if (preferred_format == "json")
        formatted_output = format_json(aliases);
    else if (preferred_format == "table")
        formatted_output = format_table(aliases);
    else if (preferred_format == "yaml")
        formatted_output = format_yaml(aliases);

    return formatted_output;
}

std::string mp::ClientFormatter::format_csv(const mp::AliasDict& aliases) const
{
    return std::string{};
}

std::string mp::ClientFormatter::format_json(const mp::AliasDict& aliases) const
{
    return std::string{};
}

std::string mp::ClientFormatter::format_table(const mp::AliasDict& aliases) const
{
    fmt::memory_buffer buf;

    if (aliases.empty())
        return "No aliases defined.\n";

    // Don't use for (const auto& elem : aliases) to preserve constness.
    for (auto dict_it = aliases.cbegin(); dict_it != aliases.cend(); ++dict_it)
    {
        const auto& name = dict_it->first;
        const auto& def = dict_it->second;

        std::ostringstream all_args;
        std::copy(def.arguments.cbegin(), def.arguments.cend(), std::ostream_iterator<std::string>(all_args, " "));

        fmt::format_to(buf, "{:<28}{:<18}{:<17}{:<}\n", name, def.instance, def.command, all_args.str());
    }

    return fmt::to_string(buf);
}

std::string mp::ClientFormatter::format_yaml(const mp::AliasDict& aliases) const
{
    YAML::Node aliases_node;

    for (auto dict_it = aliases.cbegin(); dict_it != aliases.cend(); ++dict_it)
    {
        const auto& name = dict_it->first;
        const auto& def = dict_it->second;

        YAML::Node alias_node;
        alias_node["name"] = name;
        alias_node["instance"] = def.instance;
        alias_node["command"] = def.command;

        alias_node["arguments"] = YAML::Node(YAML::NodeType::Sequence);
        for (auto arg = def.arguments.cbegin(); arg != def.arguments.cend(); arg++)
            alias_node["arguments"].push_back(*arg);

        aliases_node[name].push_back(alias_node);
    }

    return mpu::emit_yaml(aliases_node);
}
