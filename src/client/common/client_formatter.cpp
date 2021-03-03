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

#include <multipass/cli/format_utils.h>
#include <multipass/utils.h>

#include <fmt/format.h>
#include <yaml-cpp/yaml.h>

#include <sstream>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

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

std::vector<std::string> mp::ClientFormatter::escape_args(const std::vector<std::string>& args) const
{
    std::vector<std::string> escaped_args;

    auto replace = [](const std::string& s) { return QString::fromStdString(s).replace(" ", "\\ ").toStdString(); };

    std::transform(args.cbegin(), args.cend(), std::back_inserter<std::vector<std::string>>(escaped_args), replace);

    return escaped_args;
}

std::string mp::ClientFormatter::format_csv(const mp::AliasDict& aliases) const
{
    fmt::memory_buffer buf;
    fmt::format_to(buf, "Alias,Instance,Command,Args\n");

    for (auto dict_it = aliases.cbegin(); dict_it != aliases.cend(); ++dict_it)
    {
        const auto& name = dict_it->first;
        const auto& def = dict_it->second;

        fmt::format_to(buf, "{},{},{},", name, def.instance, def.command);

        fmt::format_to(buf, "\"{}\"\n", fmt::join(escape_args(def.arguments), " "));
    }

    return fmt::to_string(buf);
}

std::string mp::ClientFormatter::format_json(const mp::AliasDict& aliases) const
{
    QJsonObject aliases_json;
    QJsonArray aliases_array;

    for (auto dict_it = aliases.cbegin(); dict_it != aliases.cend(); ++dict_it)
    {
        const auto& name = dict_it->first;
        const auto& def = dict_it->second;

        QJsonObject alias_obj;
        alias_obj.insert("name", QString::fromStdString(name));
        alias_obj.insert("instance", QString::fromStdString(def.instance));
        alias_obj.insert("command", QString::fromStdString(def.command));

        QJsonArray args;
        for (const auto& arg : def.arguments)
            args.append(QString::fromStdString(arg));
        alias_obj.insert("arguments", args);

        aliases_array.append(alias_obj);
    }

    aliases_json.insert("aliases", aliases_array);

    return QString(QJsonDocument(aliases_json).toJson()).toStdString();
}

std::string mp::ClientFormatter::format_table(const mp::AliasDict& aliases) const
{
    fmt::memory_buffer buf;

    if (aliases.empty())
        return "No aliases defined.\n";

    const auto alias_width = mp::format::column_width(
        aliases.cbegin(), aliases.cend(), [](const auto& alias) -> int { return alias.first.length(); }, 7);
    const auto instance_width = mp::format::column_width(
        aliases.cbegin(), aliases.cend(), [](const auto& alias) -> int { return alias.second.instance.length(); }, 10);
    const auto command_width = mp::format::column_width(
        aliases.cbegin(), aliases.cend(), [](const auto& alias) -> int { return alias.second.command.length(); }, 9);

    const auto row_format = "{:<{}}{:<{}}{:<{}}{:<}\n";

    fmt::format_to(buf, row_format, "Alias", alias_width, "Instance", instance_width, "Command", command_width, "Args");

    // Don't use for (const auto& elem : aliases) to preserve constness.
    for (auto dict_it = aliases.cbegin(); dict_it != aliases.cend(); ++dict_it)
    {
        const auto& name = dict_it->first;
        const auto& def = dict_it->second;

        fmt::format_to(buf, row_format, name, alias_width, def.instance, instance_width, def.command, command_width,
                       fmt::join(escape_args(def.arguments), " "));
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
