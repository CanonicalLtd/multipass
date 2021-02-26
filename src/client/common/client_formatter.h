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

#ifndef MULTIPASS_CLIENT_FORMATTER_H
#define MULTIPASS_CLIENT_FORMATTER_H

#include <multipass/cli/alias_dict.h>

namespace multipass
{
class ClientFormatter
{
public:
    ClientFormatter() : preferred_format("table")
    {
    }
    ClientFormatter(const std::string& format) : preferred_format(format)
    {
    }

    std::string format(const mp::AliasDict& aliases) const;

private:
    std::string format_csv(const mp::AliasDict& aliases) const;
    std::string format_json(const mp::AliasDict& aliases) const;
    std::string format_table(const mp::AliasDict& aliases) const;
    std::string format_yaml(const mp::AliasDict& aliases) const;

    std::string preferred_format;
};
} // namespace multipass
#endif // MULTIPASS_CLIENT_FORMATTER_H
