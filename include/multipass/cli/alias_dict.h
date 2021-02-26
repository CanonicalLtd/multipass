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

#ifndef MULTIPASS_ALIAS_DICT_H
#define MULTIPASS_ALIAS_DICT_H

#include <multipass/cli/alias_definition.h>
#include <multipass/optional.h>

#include <string>
#include <unordered_map>
#include <vector>

namespace mp = multipass;

namespace multipass
{
class AliasDict
{
public:
    typedef typename std::unordered_map<std::string, AliasDefinition> DictType;

    AliasDict();
    ~AliasDict();
    void add_alias(const std::string& alias, const AliasDefinition& command);
    void remove_alias(const std::string& alias);
    mp::optional<AliasDefinition> get_alias(const std::string& alias) const;
    DictType::iterator begin();
    DictType::iterator end();
    DictType::const_iterator cbegin() const;
    DictType::const_iterator cend() const;
    bool empty() const;

private:
    bool modified = false;
    DictType aliases;
}; // class AliasDict
} // namespace multipass
#endif // MULTIPASS_ALIAS_DICT_H
