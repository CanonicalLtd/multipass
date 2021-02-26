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

#include <multipass/cli/alias_dict.h>

#include <stdexcept>

mp::AliasDict::AliasDict()
{
    // TODO: read from file
    aliases.emplace("lsp", mp::AliasDefinition{"primary", "ls", {}});
    aliases.emplace("llp", mp::AliasDefinition{"primary", "ls", {"-l", "-a"}});
}

mp::AliasDict::~AliasDict()
{
    if (modified)
    {
        // TODO: overwrite file contents with the new dictionary
    }
}

void mp::AliasDict::add_alias(const std::string& alias, const mp::AliasDefinition& command)
{
    if (aliases.try_emplace(alias, command).second)
    {
        modified = true;
    }
}

void mp::AliasDict::remove_alias(const std::string& alias)
{
    if (aliases.erase(alias) > 0)
    {
        modified = true;
    }
}

mp::optional<mp::AliasDefinition> mp::AliasDict::get_alias(const std::string& alias) const
{
    try
    {
        return aliases.at(alias);
    }
    catch (const std::out_of_range&)
    {
        return mp::nullopt;
    }
}

mp::AliasDict::DictType::iterator mp::AliasDict::begin()
{
    return aliases.begin();
}

mp::AliasDict::DictType::iterator mp::AliasDict::end()
{
    return aliases.end();
}

mp::AliasDict::DictType::const_iterator mp::AliasDict::cbegin() const
{
    return aliases.cbegin();
}

mp::AliasDict::DictType::const_iterator mp::AliasDict::cend() const
{
    return aliases.cend();
}

bool mp::AliasDict::empty() const
{
    return aliases.empty();
}
